#include <kernel/chimera_types.h>
#include <stdio.h>
#include <time.h>
#include <chimera_telemetry.h>

extern int sysinfo(void *info);

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    chimera_system_info_t si;
    if (sysinfo(&si) < 0) {
        fprintf(stderr, "uptime: system information unavailable\n");
        return 1;
    }

    uint64_t sec = si.uptime_seconds;
    uint64_t days = sec / 86400;
    uint64_t hours = (sec % 86400) / 3600;
    uint64_t mins = (sec % 3600) / 60;

    printf(" up ");
    if (days > 0) printf("%llu day%s, ", (long long)days, days > 1 ? "s" : "");
    if (hours > 0) printf("%llu:%02llu, ", (long long)hours, (long long)mins);
    else printf("%llu min%s, ", (long long)mins, mins > 1 ? "s" : "");
    printf("%u online CPU%s\n", si.cpu_count, si.cpu_count == 1 ? "" : "s");
    return 0;
}
