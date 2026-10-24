
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>

#define DEFAULT_INTERVAL_MS   10
#define DEFAULT_THRESHOLD_MS  30
#define MAX_SAVED_FREEZES     16
#define MAX_STRESS_WORKERS    16

typedef struct {
    unsigned long long total_memory;
    unsigned long long free_memory;
    unsigned int       cpu_count;
    unsigned int       uptime_seconds;
    char               os_name[32];
    char               os_version[32];
    char               kernel_name[32];
    char               architecture[16];
    char               hostname[64];
} chimera_sysinfo_t;

#define SYS_sysinfo 251

typedef struct {
    unsigned int       current_cpu;
    unsigned int       active_cpus;
    unsigned long long uptime_ms;
    unsigned long long lock_contentions;
    unsigned long long lock_slow_spins;
    unsigned long long sched_cross_spins;
    unsigned long long total_switches;
    unsigned long long total_yields;
    unsigned long long total_steals;
    unsigned long long irq_pit;
    unsigned long long irq_lapic;
    unsigned long long ipi_sched;
    unsigned long long ipi_tlb;
    unsigned long long vc_sync_count;
    unsigned long long vc_sync_last_duration_us;
} chimera_diag_stats_t;

extern int sysinfo(void *info);
extern int syscall(int number, ...);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static int get_diag_stats(chimera_diag_stats_t *out) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    return syscall(SYS_sysinfo, (unsigned long)out, 1UL, 0UL, 0UL, 0UL, 0UL);
}

static int set_cpu_affinity(int cpu) {
    return syscall(SYS_sysinfo, 0UL, 2UL, (unsigned long)cpu, 0UL, 0UL, 0UL);
}
#pragma clang diagnostic pop

static const char *classify_freeze_origin(const chimera_diag_stats_t *before,
                                          const chimera_diag_stats_t *after,
                                          char *detail, size_t detail_len) {
    if (!before || !after) {
        if (detail && detail_len > 0) detail[0] = '\0';
        return "UNKNOWN";
    }

    unsigned long long d_vc = (after->vc_sync_count >= before->vc_sync_count)
                                  ? (after->vc_sync_count - before->vc_sync_count) : 0;
    unsigned long long d_cont = (after->lock_contentions >= before->lock_contentions)
                                    ? (after->lock_contentions - before->lock_contentions) : 0;
    unsigned long long d_slow = (after->lock_slow_spins >= before->lock_slow_spins)
                                    ? (after->lock_slow_spins - before->lock_slow_spins) : 0;
    unsigned long long d_cross = (after->sched_cross_spins >= before->sched_cross_spins)
                                     ? (after->sched_cross_spins - before->sched_cross_spins) : 0;
    unsigned long long d_tlb = (after->ipi_tlb >= before->ipi_tlb)
                                   ? (after->ipi_tlb - before->ipi_tlb) : 0;
    unsigned long long d_switches = (after->total_switches >= before->total_switches)
                                        ? (after->total_switches - before->total_switches) : 0;

    if (d_vc > 0) {
        if (detail && detail_len > 0) {
            snprintf(detail, detail_len, "fb blit %llu.%01llu ms, count +%llu",
                     after->vc_sync_last_duration_us / 1000ULL,
                     (after->vc_sync_last_duration_us % 1000ULL) / 100ULL,
                     d_vc);
        }
        return "VC_SCROLL_MMIO";
    }

    if (d_slow > 0 || d_cross > 0 || d_cont >= 20) {
        if (detail && detail_len > 0) {
            snprintf(detail, detail_len, "contention +%llu, slow-spins +%llu, cross-spins +%llu",
                     d_cont, d_slow, d_cross);
        }
        return "LOCK_CONTENTION";
    }

    if (d_tlb > 0) {
        if (detail && detail_len > 0) {
            snprintf(detail, detail_len, "tlb shootdowns +%llu", d_tlb);
        }
        return "TLB_SHOOTDOWN";
    }

    if (after->current_cpu != before->current_cpu) {
        if (detail && detail_len > 0) {
            snprintf(detail, detail_len, "migrated cpu %u -> %u",
                     before->current_cpu, after->current_cpu);
        }
        return "CORE_MIGRATION";
    }

    if (d_switches > 1) {
        if (detail && detail_len > 0) {
            snprintf(detail, detail_len, "preempted, context switches +%llu", d_switches);
        }
        return "SCHED_PREEMPTION";
    }

    if (detail && detail_len > 0) {
        unsigned long long d_pit = (after->irq_pit >= before->irq_pit)
                                       ? (after->irq_pit - before->irq_pit) : 0;
        unsigned long long d_lapic = (after->irq_lapic >= before->irq_lapic)
                                         ? (after->irq_lapic - before->irq_lapic) : 0;
        snprintf(detail, detail_len, "wake delay, pit +%llu, lapic +%llu", d_pit, d_lapic);
    }
    return "TIMER_JITTER";
}

typedef enum {
    MODE_SLEEP = 0,
    MODE_SPIN,
    MODE_SYSCALL
} monitor_mode_t;

typedef struct {
    unsigned long long cycle;
    unsigned long long offset_ms;
    unsigned long long duration_us;
    unsigned long long jitter_us;
    char origin[24];
    char detail[64];
} freeze_record_t;

static volatile int g_running = 1;
static pid_t g_workers[MAX_STRESS_WORKERS];
static int g_num_workers = 0;

static void sigint_handler(int sig) {
    (void)sig;
    g_running = 0;
}

static unsigned long long time_now_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)tv.tv_sec * 1000000ULL + (unsigned long long)tv.tv_usec;
}

static void print_usage(const char *prog) {
    printf("Usage: %s [options]\n\n", prog);
    printf("Real-time cyclic freeze and latency spike detector for Chimera OS.\n\n");
    printf("Options:\n");
    printf("  -i <ms>       Target cycle interval in ms (default: %d)\n", DEFAULT_INTERVAL_MS);
    printf("  -t <ms>       Freeze detection threshold in ms (default: %d)\n", DEFAULT_THRESHOLD_MS);
    printf("  -c <cpu>      Pin monitor to specific CPU core (0..N-1, default: unpinned)\n");
    printf("  -m <mode>     Test mode: 'sleep' (default), 'spin', 'syscall'\n");
    printf("  -s <workers>  Spawn N background SMP stress workers (0-%d)\n", MAX_STRESS_WORKERS);
    printf("  -n <cycles>   Exit after N cycles (default: 0 = continuous until Ctrl+C)\n");
    printf("  -q            Quiet mode (only print detected freeze events)\n");
    printf("  -h, --help    Show this help message\n\n");
    printf("Modes:\n");
    printf("  sleep         Cyclic usleep() to detect scheduler wake delays & missed ticks\n");
    printf("  spin          Tight busy-loop to detect CPU stalls and IRQ disabling lockups\n");
    printf("  syscall       Repeated gettimeofday() calls to detect kernel lock contention\n\n");
}

static void cleanup_workers(void) {
    for (int i = 0; i < g_num_workers; i++) {
        if (g_workers[i] > 0) {
            kill(g_workers[i], SIGTERM);
            int status = 0;
            waitpid(g_workers[i], &status, 0);
        }
    }
    g_num_workers = 0;
}

static void run_stress_worker(int id) {
    (void)id;
    volatile unsigned long long val = 0x9E3779B97F4A7C15ULL;
    while (1) {
        for (int i = 0; i < 50000; i++) {
            val = (val * 6364136223846793005ULL) + 1442695040888963407ULL;
        }
        usleep(1000);
    }
}

static void make_bar(char *buf, int buf_len, unsigned long long count, unsigned long long total) {
    int bar_width = 20;
    if (total == 0 || buf_len < bar_width + 3) {
        if (buf_len > 0) buf[0] = '\0';
        return;
    }
    int filled = (int)((count * (unsigned long long)bar_width) / total);
    if (filled > bar_width) filled = bar_width;
    buf[0] = '[';
    for (int i = 0; i < bar_width; i++) {
        buf[1 + i] = (i < filled) ? '#' : ' ';
    }
    buf[1 + bar_width] = ']';
    buf[2 + bar_width] = '\0';
}

int main(int argc, char **argv) {
    unsigned int interval_ms = DEFAULT_INTERVAL_MS;
    unsigned int threshold_ms = DEFAULT_THRESHOLD_MS;
    monitor_mode_t mode = MODE_SLEEP;
    const char *mode_name = "sleep";
    int stress_count = 0;
    int pin_cpu = -1;
    unsigned long long max_cycles = 0;
    int quiet = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            interval_ms = (unsigned int)atoi(argv[++i]);
            if (interval_ms < 1) interval_ms = 1;
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            threshold_ms = (unsigned int)atoi(argv[++i]);
            if (threshold_ms < 1) threshold_ms = 1;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            pin_cpu = atoi(argv[++i]);
            if (pin_cpu < 0) pin_cpu = -1;
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            const char *m = argv[++i];
            if (strcmp(m, "sleep") == 0) {
                mode = MODE_SLEEP;
                mode_name = "sleep";
            } else if (strcmp(m, "spin") == 0) {
                mode = MODE_SPIN;
                mode_name = "spin";
            } else if (strcmp(m, "syscall") == 0) {
                mode = MODE_SYSCALL;
                mode_name = "syscall";
            } else {
                printf("Unknown mode: %s. Using default 'sleep'.\n", m);
            }
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            stress_count = atoi(argv[++i]);
            if (stress_count < 0) stress_count = 0;
            if (stress_count > MAX_STRESS_WORKERS) stress_count = MAX_STRESS_WORKERS;
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            max_cycles = (unsigned long long)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-q") == 0) {
            quiet = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    chimera_sysinfo_t si;
    memset(&si, 0, sizeof(si));
    int cpu_count = 1;
    if (sysinfo(&si) == 0 && si.cpu_count > 0) {
        cpu_count = (int)si.cpu_count;
    }

    if (pin_cpu >= 0) {
        if (pin_cpu >= cpu_count) {
            printf("Warning: CPU %d exceeds active CPU count (%d). Clamping to %d.\n",
                   pin_cpu, cpu_count, cpu_count - 1);
            pin_cpu = cpu_count - 1;
        }
        set_cpu_affinity(pin_cpu);
    }

    chimera_diag_stats_t test_diag;
    int diag_supported = (get_diag_stats(&test_diag) == 0 && test_diag.active_cpus > 0);

    printf("============================================================\n");
    printf("   Chimera OS Real-Time Freeze Monitor (freezemon)\n");
    printf("============================================================\n");
    printf("  Mode:        %s\n", mode_name);
    printf("  Interval:    %u ms\n", interval_ms);
    printf("  Threshold:   %u ms\n", threshold_ms);
    if (pin_cpu >= 0) {
        printf("  CPU Core:    Pinned to CPU %d (of %d active)\n", pin_cpu, cpu_count);
    } else {
        printf("  SMP Cores:   %d active (unpinned)\n", cpu_count);
    }
    printf("  Origin Diag: %s\n", diag_supported ? "Active (Kernel Telemetry)" : "Unavailable");
    printf("  PID:         %d\n", getpid());

    if (stress_count > 0) {
        printf("  Workers:     Spawning %d SMP stress workers...\n", stress_count);
        for (int i = 0; i < stress_count; i++) {
            pid_t p = fork();
            if (p == 0) {
                run_stress_worker(i + 1);
                exit(0);
            } else if (p > 0) {
                g_workers[g_num_workers++] = p;
            }
        }
    }
    printf("------------------------------------------------------------\n");
    printf("Monitoring system latency... (Press Ctrl+C to stop)\n\n");

    unsigned long long target_us = (unsigned long long)interval_ms * 1000ULL;
    unsigned long long threshold_us = (unsigned long long)threshold_ms * 1000ULL;

    unsigned long long total_cycles = 0;
    unsigned long long freeze_count = 0;
    unsigned long long min_latency_us = 0xFFFFFFFFFFFFFFFFULL;
    unsigned long long max_latency_us = 0;
    unsigned long long sum_latency_us = 0;

    unsigned long long buckets[5] = {0, 0, 0, 0, 0};

    freeze_record_t worst_freezes[MAX_SAVED_FREEZES];
    int saved_freeze_count = 0;

    unsigned long long start_time_us = time_now_us();
    unsigned long long last_heartbeat_us = start_time_us;

    while (g_running) {
        if (max_cycles > 0 && total_cycles >= max_cycles) {
            break;
        }

        chimera_diag_stats_t s_before;
        if (diag_supported) {
            get_diag_stats(&s_before);
        }

        unsigned long long t0 = time_now_us();

        if (mode == MODE_SLEEP) {
            usleep((unsigned int)interval_ms * 1000);
        } else if (mode == MODE_SPIN) {
            unsigned long long spin_target = t0 + target_us;
            while (g_running && time_now_us() < spin_target) {
                __asm__ volatile("pause");
            }
        } else if (mode == MODE_SYSCALL) {
            for (int k = 0; k < 20; k++) {
                struct timeval tv_dummy;
                gettimeofday(&tv_dummy, NULL);
            }
            unsigned long long now_spin = time_now_us();
            if (now_spin < t0 + target_us) {
                usleep((unsigned int)((t0 + target_us - now_spin)));
            }
        }

        unsigned long long t1 = time_now_us();
        unsigned long long elapsed_us = (t1 >= t0) ? (t1 - t0) : target_us;
        total_cycles++;
        sum_latency_us += elapsed_us;

        if (elapsed_us < min_latency_us) min_latency_us = elapsed_us;
        if (elapsed_us > max_latency_us) max_latency_us = elapsed_us;

        unsigned long long b_us = elapsed_us;
        if (b_us < target_us + 5000ULL) {
            buckets[0]++;
        } else if (b_us < target_us + 15000ULL) {
            buckets[1]++;
        } else if (b_us < target_us + 40000ULL) {
            buckets[2]++;
        } else if (b_us < 100000ULL) {
            buckets[3]++;
        } else {
            buckets[4]++;
        }

        if (elapsed_us >= threshold_us) {
            freeze_count++;
            unsigned long long jitter_us = (elapsed_us > target_us) ? (elapsed_us - target_us) : 0;
            unsigned long long offset_ms = (t1 - start_time_us) / 1000ULL;
            unsigned long long sec = offset_ms / 1000ULL;
            unsigned long long min = sec / 60ULL;
            sec %= 60ULL;
            unsigned long long hr = min / 60ULL;
            min %= 60ULL;

            chimera_diag_stats_t s_after;
            const char *origin = "UNKNOWN";
            char detail[64] = {0};
            if (diag_supported && get_diag_stats(&s_after) == 0) {
                origin = classify_freeze_origin(&s_before, &s_after, detail, sizeof(detail));
            }

            static unsigned long long s_last_alert_us = 0;
            if (t1 - s_last_alert_us >= 500000ULL || s_last_alert_us == 0) {
                s_last_alert_us = t1;
                printf("\033[1;31m[FREEZE DETECTED]\033[0m cycle #%llu at +%02llu:%02llu:%02llu: "
                       "took %llu.%03llu ms (expected %u ms, jitter +%llu.%03llu ms)\n",
                       total_cycles, hr, min, sec,
                       elapsed_us / 1000ULL, elapsed_us % 1000ULL,
                       interval_ms,
                       jitter_us / 1000ULL, jitter_us % 1000ULL);
                if (diag_supported) {
                    unsigned long long d_sw = (s_after.total_switches >= s_before.total_switches)
                                                  ? (s_after.total_switches - s_before.total_switches) : 0;
                    printf("  \033[36m-> origin:\033[0m \033[1;33m%s\033[0m (%s) | cpu: %u | cs: +%llu\n",
                           origin, detail, s_after.current_cpu, d_sw);
                }
            }

            if (saved_freeze_count < MAX_SAVED_FREEZES) {
                worst_freezes[saved_freeze_count].cycle = total_cycles;
                worst_freezes[saved_freeze_count].offset_ms = offset_ms;
                worst_freezes[saved_freeze_count].duration_us = elapsed_us;
                worst_freezes[saved_freeze_count].jitter_us = jitter_us;
                snprintf(worst_freezes[saved_freeze_count].origin, sizeof(worst_freezes[saved_freeze_count].origin), "%s", origin);
                snprintf(worst_freezes[saved_freeze_count].detail, sizeof(worst_freezes[saved_freeze_count].detail), "%s", detail);
                saved_freeze_count++;
            } else {
                int min_idx = 0;
                for (int j = 1; j < MAX_SAVED_FREEZES; j++) {
                    if (worst_freezes[j].duration_us < worst_freezes[min_idx].duration_us) {
                        min_idx = j;
                    }
                }
                if (elapsed_us > worst_freezes[min_idx].duration_us) {
                    worst_freezes[min_idx].cycle = total_cycles;
                    worst_freezes[min_idx].offset_ms = offset_ms;
                    worst_freezes[min_idx].duration_us = elapsed_us;
                    worst_freezes[min_idx].jitter_us = jitter_us;
                    snprintf(worst_freezes[min_idx].origin, sizeof(worst_freezes[min_idx].origin), "%s", origin);
                    snprintf(worst_freezes[min_idx].detail, sizeof(worst_freezes[min_idx].detail), "%s", detail);
                }
            }
        }

        if (!quiet && (t1 - last_heartbeat_us >= 1000000ULL)) {
            last_heartbeat_us = t1;
            unsigned long long offset_ms = (t1 - start_time_us) / 1000ULL;
            unsigned long long sec = offset_ms / 1000ULL;
            unsigned long long min = sec / 60ULL;
            sec %= 60ULL;
            unsigned long long hr = min / 60ULL;
            min %= 60ULL;

            unsigned long long avg_us = total_cycles > 0 ? (sum_latency_us / total_cycles) : 0;
            printf("[freezemon] +%02llu:%02llu:%02llu | cycles: %6llu | cur: %3llu.%03llu ms | "
                   "min/avg/max: %llu.%03llu / %llu.%03llu / %llu.%03llu ms | freezes: %llu\n",
                   hr, min, sec, total_cycles,
                   elapsed_us / 1000ULL, elapsed_us % 1000ULL,
                   (min_latency_us == 0xFFFFFFFFFFFFFFFFULL ? 0 : min_latency_us / 1000ULL),
                   (min_latency_us == 0xFFFFFFFFFFFFFFFFULL ? 0 : min_latency_us % 1000ULL),
                   avg_us / 1000ULL, avg_us % 1000ULL,
                   max_latency_us / 1000ULL, max_latency_us % 1000ULL,
                   freeze_count);
        }
    }

    cleanup_workers();

    unsigned long long end_time_us = time_now_us();
    unsigned long long total_duration_us = (end_time_us >= start_time_us) ? (end_time_us - start_time_us) : 0;

    printf("\n============================================================\n");
    printf("        Chimera OS Freeze Monitor Session Summary\n");
    printf("============================================================\n");
    printf("Duration:          %llu.%03llu s (%llu cycles)\n",
           total_duration_us / 1000000ULL, (total_duration_us % 1000000ULL) / 1000ULL, total_cycles);
    printf("Test Mode:         %s (interval %u ms, threshold %u ms)\n", mode_name, interval_ms, threshold_ms);

    if (total_cycles > 0) {
        unsigned long long avg_us = sum_latency_us / total_cycles;
        printf("Min Latency:       %llu.%03llu ms\n",
               min_latency_us / 1000ULL, min_latency_us % 1000ULL);
        printf("Avg Latency:       %llu.%03llu ms\n",
               avg_us / 1000ULL, avg_us % 1000ULL);
        printf("Max Latency:       %llu.%03llu ms\n",
               max_latency_us / 1000ULL, max_latency_us % 1000ULL);
        printf("Freezes (>%ums):    %llu\n", threshold_ms, freeze_count);

        printf("\nLatency Distribution:\n");
        char bar[32];
        make_bar(bar, sizeof(bar), buckets[0], total_cycles);
        printf("  < %2u ms:        %6llu  %s\n", interval_ms + 5, buckets[0], bar);
        make_bar(bar, sizeof(bar), buckets[1], total_cycles);
        printf("  %2u - %2u ms:    %6llu  %s\n", interval_ms + 5, interval_ms + 15, buckets[1], bar);
        make_bar(bar, sizeof(bar), buckets[2], total_cycles);
        printf("  %2u - %2u ms:    %6llu  %s\n", interval_ms + 15, interval_ms + 40, buckets[2], bar);
        make_bar(bar, sizeof(bar), buckets[3], total_cycles);
        printf("  %2u - 100 ms:    %6llu  %s\n", interval_ms + 40, buckets[3], bar);
        make_bar(bar, sizeof(bar), buckets[4], total_cycles);
        printf("  >= 100 ms:       %6llu  %s\n", buckets[4], bar);

        if (saved_freeze_count > 0) {
            for (int i = 0; i < saved_freeze_count - 1; i++) {
                for (int j = i + 1; j < saved_freeze_count; j++) {
                    if (worst_freezes[j].duration_us > worst_freezes[i].duration_us) {
                        freeze_record_t tmp = worst_freezes[i];
                        worst_freezes[i] = worst_freezes[j];
                        worst_freezes[j] = tmp;
                    }
                }
            }

            printf("\nTop Worst Freeze Events:\n");
            for (int i = 0; i < saved_freeze_count && i < 8; i++) {
                unsigned long long sec = worst_freezes[i].offset_ms / 1000ULL;
                unsigned long long min = sec / 60ULL;
                sec %= 60ULL;
                unsigned long long hr = min / 60ULL;
                min %= 60ULL;
                printf("  #%d: %llu.%03llu ms (+%llu.%03llu ms jitter) at +%02llu:%02llu:%02llu (cycle #%llu) [%s: %s]\n",
                       i + 1,
                       worst_freezes[i].duration_us / 1000ULL, worst_freezes[i].duration_us % 1000ULL,
                       worst_freezes[i].jitter_us / 1000ULL, worst_freezes[i].jitter_us % 1000ULL,
                       hr, min, sec,
                       worst_freezes[i].cycle,
                       worst_freezes[i].origin[0] ? worst_freezes[i].origin : "NONE",
                       worst_freezes[i].detail[0] ? worst_freezes[i].detail : "-");
            }
        }
    }
    printf("============================================================\n");
    return 0;
}

