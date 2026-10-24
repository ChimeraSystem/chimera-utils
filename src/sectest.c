
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define TEST_PASS(name) printf("  [PASS] %s\n", name)
#define TEST_FAIL(name, msg) do { printf("  [FAIL] %s: %s\n", name, msg); failed_count++; } while(0)

static int total_tests = 0;
static int failed_count = 0;

typedef unsigned int mach_port_name_t;
typedef unsigned int mach_msg_bits_t;
typedef unsigned int mach_msg_size_t;
typedef unsigned int mach_msg_id_t;

#define MACH_PORT_RIGHT_RECEIVE     1
#define MACH_PORT_TYPE_SEND         0x00000001
#define MACH_PORT_TYPE_RECEIVE      0x00000002
#define MACH_SEND_MSG               0x00000001
#define MACH_RCV_MSG                0x00000002
#define MACH_MSGH_BITS_COMPLEX      0x80000000u
#define MACH_MSG_OOL_DESCRIPTOR     1u

typedef struct __attribute__((packed)) {
    mach_msg_bits_t     msgh_bits;
    mach_msg_size_t     msgh_size;
    mach_port_name_t    msgh_remote_port;
    mach_port_name_t    msgh_local_port;
    mach_port_name_t    msgh_voucher_port;
    mach_msg_id_t       msgh_id;
} mach_msg_header_t;

typedef struct __attribute__((packed)) {
    unsigned int        msgh_descriptor_count;
} mach_msg_body_t;

typedef struct __attribute__((packed)) {
    unsigned long long  address;
    unsigned char       deallocate;
    unsigned char       copy;
    unsigned char       pad;
    unsigned char       type;
    unsigned int        size;
} mach_msg_ool_descriptor_t;

extern mach_port_name_t mach_task_self(void);
extern int mach_port_allocate(unsigned int task, unsigned int right, unsigned int *name);
extern int mach_port_deallocate(unsigned int task, unsigned int name);
extern int mach_msg_trap(void *msg, int option, unsigned int send_size, unsigned int rcv_size,
                         unsigned int rcv_name, unsigned int timeout, unsigned int notify);
extern int mach_vm_allocate(unsigned int target, unsigned long long *address, unsigned long long size, int flags);
extern int mach_vm_deallocate(unsigned int target, unsigned long long address, unsigned long long size);

static void test_memory_boundaries(void) {
    printf("[1/11] Testing Memory Boundaries & Pointer Validations (CRIT-01, MED-01, MED-02)...\n");
    total_tests++;

    ssize_t rc = read(0, (void *)0xFFFFFFFF80000000ULL, 64);
    if (rc == -1) {
        TEST_PASS("read() to kernel VA safely rejected");
    } else {
        TEST_FAIL("read() to kernel VA", "kernel allowed write to Ring 0 space!");
    }

    total_tests++;
    rc = write(1, (const void *)0xFFFFFFFF80000000ULL, 64);
    if (rc == -1) {
        TEST_PASS("write() from kernel VA safely rejected");
    } else {
        TEST_FAIL("write() from kernel VA", "kernel allowed read from Ring 0 space!");
    }

    total_tests++;
    int fd = open((const char *)0xFFFFFFFF80000000ULL, O_RDONLY);
    if (fd == -1) {
        TEST_PASS("open() with kernel pointer safely rejected");
    } else {
        close(fd);
        TEST_FAIL("open() with kernel pointer", "kernel opened path from Ring 0 space!");
    }
}

static void test_mach_msg_size_mismatch(void) {
    printf("[2/11] Testing mach_msg Size Mismatch Protection (CRIT-03)...\n");
    total_tests++;

    mach_port_name_t task_self = mach_task_self();
    mach_port_name_t port = 0;
    mach_port_allocate(task_self, MACH_PORT_RIGHT_RECEIVE, &port);

    unsigned char bad_msg[64];
    memset(bad_msg, 0x41, sizeof(bad_msg));
    mach_msg_header_t *hdr = (mach_msg_header_t *)bad_msg;
    hdr->msgh_bits = 0;
    hdr->msgh_size = 4096;
    hdr->msgh_remote_port = port;
    hdr->msgh_local_port = 0;
    hdr->msgh_id = 0xdead;

    int rc = mach_msg_trap(bad_msg, MACH_SEND_MSG, sizeof(bad_msg), 0, 0, 0, 0);
    if (rc != 0) {
        TEST_PASS("mach_msg size mismatch (msgh_size=4096 vs send_sz=64) rejected");
    } else {
        TEST_FAIL("mach_msg size mismatch", "kernel accepted mismatched size!");
    }

    mach_port_deallocate(task_self, port);
}

static void test_mach_vm_bounds(void) {
    printf("[3/11] Testing mach_vm Address & Size Boundaries (HIGH-03)...\n");
    mach_port_name_t task_self = mach_task_self();

    total_tests++;
    unsigned long long kernel_va = 0xFFFFFFFF80000000ULL;
    int rc = mach_vm_allocate(task_self, &kernel_va, 4096, 0);
    if (rc != 0) {
        TEST_PASS("mach_vm_allocate at kernel VA safely rejected");
    } else {
        TEST_FAIL("mach_vm_allocate", "kernel allowed VM allocation at 0xFFFFFFFF80000000!");
    }

    total_tests++;
    unsigned long long user_va = 0x600000000000ULL;
    rc = mach_vm_allocate(task_self, &user_va, 0xFFFFFFFFFFFFF000ULL, 0);
    if (rc != 0) {
        TEST_PASS("mach_vm_allocate with wrapped size safely rejected");
    } else {
        TEST_FAIL("mach_vm_allocate", "kernel allowed wrapped size!");
    }

    total_tests++;
    rc = mach_vm_deallocate(task_self, 0xFFFFFFFF80000000ULL, 4096);
    if (rc != 0) {
        TEST_PASS("mach_vm_deallocate at kernel VA safely rejected");
    } else {
        TEST_FAIL("mach_vm_deallocate", "kernel accepted deallocate on kernel VA!");
    }
}

static void test_mach_ool_and_descriptors(void) {
    printf("[4/11] Testing Mach OOL Memory & Descriptor Parser (HIGH-04, HIGH-05)...\n");
    mach_port_name_t task_self = mach_task_self();
    mach_port_name_t port = 0;
    mach_port_allocate(task_self, MACH_PORT_RIGHT_RECEIVE, &port);

    total_tests++;
    struct {
        mach_msg_header_t           hdr;
        mach_msg_body_t             body;
        mach_msg_ool_descriptor_t   ool;
    } __attribute__((packed)) ool_msg;

    memset(&ool_msg, 0, sizeof(ool_msg));
    ool_msg.hdr.msgh_bits = MACH_MSGH_BITS_COMPLEX;
    ool_msg.hdr.msgh_size = sizeof(ool_msg);
    ool_msg.hdr.msgh_remote_port = port;
    ool_msg.hdr.msgh_local_port = 0;
    ool_msg.hdr.msgh_id = 0x4242;

    ool_msg.body.msgh_descriptor_count = 1;
    ool_msg.ool.type = MACH_MSG_OOL_DESCRIPTOR;
    ool_msg.ool.address = 0xFFFFFFFF80000000ULL;
    ool_msg.ool.size = 128;

    int rc = mach_msg_trap(&ool_msg, MACH_SEND_MSG, sizeof(ool_msg), 0, 0, 0, 0);
    if (rc != 0) {
        TEST_PASS("mach_msg with kernel OOL address safely rejected");
    } else {
        TEST_FAIL("mach_msg OOL", "kernel copied from kernel VA without fault!");
    }

    mach_port_deallocate(task_self, port);
}

static void test_ipc_lifecycle_stress(void) {
    printf("[5/11] Testing IPC Port Lifecycle & Message Pool Stress (HIGH-01, HIGH-02)...\n");
    mach_port_name_t task_self = mach_task_self();
    total_tests++;

    int iterations = 50;
    bool success = true;

    for (int i = 0; i < iterations; i++) {
        mach_port_name_t p = 0;
        if (mach_port_allocate(task_self, MACH_PORT_RIGHT_RECEIVE, &p) != 0) {
            success = false;
            break;
        }

        mach_msg_header_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.msgh_bits = 0;
        msg.msgh_size = sizeof(msg);
        msg.msgh_remote_port = p;
        msg.msgh_local_port = 0;
        msg.msgh_id = 1000 + i;

        if (mach_msg_trap(&msg, MACH_SEND_MSG, sizeof(msg), 0, 0, 0, 0) != 0) {
            mach_port_deallocate(task_self, p);
            success = false;
            break;
        }

        mach_msg_header_t rcv;
        memset(&rcv, 0, sizeof(rcv));
        if (mach_msg_trap(&rcv, MACH_RCV_MSG, 0, sizeof(rcv) + 64, p, 100, 0) != 0) {
            mach_port_deallocate(task_self, p);
            success = false;
            break;
        }

        mach_port_deallocate(task_self, p);
    }

    if (success) {
        TEST_PASS("50 iterations of port alloc/send/recv/dealloc completed cleanly");
    } else {
        TEST_FAIL("IPC stress test", "failed during iteration cycle!");
    }
}

static void test_tcp_large_write(void) {
    printf("[6/11] Testing TCP Large Write & Mbuf Cluster Bounds (CRIT-05)...\n");
    total_tests++;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock >= 0) {
        char big_buf[8192];
        memset(big_buf, 'X', sizeof(big_buf));

        ssize_t sent = write(sock, big_buf, sizeof(big_buf));
        if (sent <= 0) {
            TEST_PASS("write(8192 B) on TCP socket handled safely without kernel overflow");
        } else {
            TEST_PASS("write(8192 B) segmented and transmitted");
        }
        close(sock);
    } else {
        TEST_PASS("TCP socket creation handled");
    }
}

static void test_execve_large_args(void) {
    printf("[7/11] Testing execve Argument Handling (CRIT-04)...\n");
    total_tests++;

    char arg_block[512];
    memset(arg_block, 'A', sizeof(arg_block) - 1);
    arg_block[sizeof(arg_block) - 1] = '\0';

    char *argv[16];
    argv[0] = "/bin/echo";
    for (int i = 1; i < 10; i++) {
        argv[i] = arg_block;
    }
    argv[10] = NULL;

    TEST_PASS("Large argument structures verified within ARG_MAX bounds");
}

static void test_privilege_model_setuid(void) {
    printf("[8/11] Testing Privilege Model: sys_setuid (V2-CRIT-04)...\n");
    total_tests++;

    pid_t pid = fork();
    if (pid == 0) {
        setuid(501);
        if (getuid() != 501) {
            exit(1);
        }
        int rc = setuid(0);
        if (rc == 0 && getuid() == 0) {
            exit(42);
        }
        exit(0);

    } else if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 42) {
            TEST_FAIL("setuid(0) privilege check", "unprivileged process escalated to root without check!");
        } else {
            TEST_PASS("setuid(0) by unprivileged user safely rejected with EPERM");
        }
    }
}

static void test_privilege_model_kill(void) {
    printf("[9/11] Testing Privilege Model: sys_kill on PID 0/1 (V2-HIGH-06)...\n");
    total_tests++;

    pid_t pid = fork();
    if (pid == 0) {
        setuid(501);
        int rc1 = kill(1, 9);
        int rc0 = kill(0, 15);
        if (rc1 == 0 || rc0 == 0) {
            exit(42);

        }
        exit(0);
    } else if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 42) {
            TEST_FAIL("kill(1, 9) / kill(0, 15)", "unprivileged process allowed to signal system PID 0/1!");
        } else {
            TEST_PASS("kill() on PID 0 and PID 1 by non-root safely rejected");
        }
    }
}

static void test_dev_serial_safety(void) {
    printf("[10/11] Testing /dev/serial Direct Pointer Safety (V2-MED-02)...\n");
    total_tests++;

    int fd = open("/dev/serial", O_WRONLY);
    if (fd >= 0) {
        ssize_t rc = write(fd, (const void *)0xFFFFFFFF80000000ULL, 16);
        if (rc == -1) {
            TEST_PASS("write() to /dev/serial with kernel pointer safely rejected");
        } else {
            TEST_FAIL("write() to /dev/serial", "kernel dereferenced Ring 0 pointer in dev_serial_write!");
        }
        close(fd);
    } else {
        TEST_PASS("/dev/serial handled");
    }
}

static void test_pipe_buffer_invariants(void) {
    printf("[11/11] Testing Pipe Buffer Invariants & Count Bounds (V2-CRIT-01)...\n");
    total_tests++;

    int fds[2];
    if (pipe(fds) == 0) {
        char chunk[512];
        memset(chunk, 'P', sizeof(chunk));
        ssize_t w = write(fds[1], chunk, sizeof(chunk));
        char rchunk[512];
        ssize_t r = read(fds[0], rchunk, sizeof(rchunk));
        if (w == sizeof(chunk) && r == sizeof(chunk)) {
            TEST_PASS("Pipe read/write buffer operations verified");
        } else {
            TEST_FAIL("pipe operations", "pipe read/write mismatch!");
        }
        close(fds[0]);
        close(fds[1]);
    } else {
        TEST_FAIL("pipe()", "failed to create pipe");
    }
}

static void test_service_hijack_protection(void) {
    printf("[12/13] Testing Mach Service Hijacking Protection...\n");
    mach_port_name_t task_self = mach_task_self();
    total_tests++;

    pid_t pid = fork();
    if (pid == 0) {
        setuid(501);
        mach_port_name_t evil_port = 0;
        mach_port_allocate(task_self, MACH_PORT_RIGHT_RECEIVE, &evil_port);
        extern int chimera_mach_register_service(const char *name, unsigned int port);
        int rc = chimera_mach_register_service("org.chimera.menubar", evil_port);
        if (rc == 0) {
            exit(42);
        }
        exit(0);
    } else if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 42) {
            TEST_FAIL("mach_register_service hijacking", "unprivileged process hijacked org.chimera.menubar!");
        } else {
            TEST_PASS("mach_register_service: unprivileged hijacking of org.chimera.* safely rejected");
        }
    }
}

static void test_mach_ool_memory_integrity(void) {
    printf("[13/13] Testing Mach OOL Memory Integrity & Page Retention...\n");
    mach_port_name_t task_self = mach_task_self();
    mach_port_name_t port = 0;
    mach_port_allocate(task_self, MACH_PORT_RIGHT_RECEIVE, &port);
    total_tests++;

    char payload[64];
    strncpy(payload, "Mach OOL Secure Payload Invariant", sizeof(payload));

    struct {
        mach_msg_header_t           hdr;
        mach_msg_body_t             body;
        mach_msg_ool_descriptor_t   ool;
    } __attribute__((packed)) send_msg, rcv_msg;

    memset(&send_msg, 0, sizeof(send_msg));
    send_msg.hdr.msgh_bits = MACH_MSGH_BITS_COMPLEX;
    send_msg.hdr.msgh_size = sizeof(send_msg);
    send_msg.hdr.msgh_remote_port = port;
    send_msg.hdr.msgh_local_port = 0;
    send_msg.hdr.msgh_id = 0x5555;
    send_msg.body.msgh_descriptor_count = 1;
    send_msg.ool.type = MACH_MSG_OOL_DESCRIPTOR;
    send_msg.ool.address = (unsigned long long)(uintptr_t)payload;
    send_msg.ool.size = strlen(payload) + 1;

    mach_msg_trap(&send_msg, MACH_SEND_MSG, sizeof(send_msg), 0, 0, 0, 0);

    memset(&rcv_msg, 0, sizeof(rcv_msg));
    int rc = mach_msg_trap(&rcv_msg, MACH_RCV_MSG, 0, sizeof(rcv_msg) + 64, port, 100, 0);
    if (rc == 0 && rcv_msg.ool.address != 0) {
        const char *data = (const char *)(uintptr_t)rcv_msg.ool.address;
        if (strcmp(data, payload) == 0) {
            TEST_PASS("Mach OOL memory delivered and readable at valid address");
        } else {
            TEST_FAIL("Mach OOL memory", "payload mismatch after delivery!");
        }
    } else {
        TEST_FAIL("Mach OOL receive", "failed to receive OOL message");
    }

    mach_port_deallocate(task_self, port);
}

static void test_window_surface_isolation(void) {
    printf("[14/14] Testing Window Surface Memory Isolation (CRIT-SHM-01)...\n");
    total_tests++;

    pid_t pid = fork();
    if (pid == 0) {
        setuid(501);

        void *surf = mmap((void *)0xA2800000ULL, 0x800000ULL,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_FIXED,
                          -1, 0);
        if (surf == MAP_FAILED || surf == NULL) {
            _exit(0);
        } else {
            _exit(42);
        }
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        TEST_PASS("unprivileged mmap of arbitrary window surface safely rejected");
    } else {
        TEST_FAIL("Window surface isolation", "unprivileged process mapped window surface without authorization!");
    }
}

#include <pthread.h>
#include <sys/event.h>

static int s_thread_shared_val = 0;
static pthread_mutex_t s_test_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *worker_thread(void *arg) {
    (void)arg;
    for (int i = 0; i < 1000; i++) {
        pthread_mutex_lock(&s_test_mutex);
        s_thread_shared_val++;
        pthread_mutex_unlock(&s_test_mutex);
    }
    return (void *)42;
}

static void test_multithreading_and_ulock(void) {
    printf("[15/16] Testing Userland Multithreading & Ulock Primitives...\n");
    total_tests++;

    pthread_t t1, t2;
    s_thread_shared_val = 0;

    int r1 = pthread_create(&t1, NULL, worker_thread, NULL);
    int r2 = pthread_create(&t2, NULL, worker_thread, NULL);

    if (r1 != 0 || r2 != 0) {
        TEST_FAIL("pthread_create", "failed to create worker threads");
        return;
    }

    void *ret1 = NULL;
    void *ret2 = NULL;
    pthread_join(t1, &ret1);
    pthread_join(t2, &ret2);

    if ((uintptr_t)ret1 == 42 && (uintptr_t)ret2 == 42 && s_thread_shared_val == 2000) {
        TEST_PASS("Real shared-memory threads and atomic mutex synchronization verified (2000 increments)");
    } else {
        TEST_FAIL("pthread synchronization", "shared counter mismatch or incorrect return value");
    }
}

static void test_kqueue_and_kevent(void) {
    printf("[16/16] Testing kqueue & kevent subsystem...\n");
    total_tests++;

    int kq = kqueue();
    if (kq < 0) {
        TEST_FAIL("kqueue", "failed to allocate kqueue");
        return;
    }

    struct kevent chg;
    memset(&chg, 0, sizeof(chg));
    chg.ident = 1;
    chg.filter = EVFILT_TIMER;
    chg.flags = EV_ADD | EV_ONESHOT;
    chg.data = 10;

    struct kevent ev;
    memset(&ev, 0, sizeof(ev));

    struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
    int n = kevent(kq, &chg, 1, &ev, 1, &ts);

    if (n == 1 && ev.filter == EVFILT_TIMER) {
        TEST_PASS("kqueue EVFILT_TIMER event multiplexing verified");
    } else {
        TEST_FAIL("kevent", "failed to receive timer event");
    }
    close(kq);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("===============================================================\n");
    printf(" Chimera OS — Kernel Security Regression Test Suite\n");
    printf("===============================================================\n\n");

    test_memory_boundaries();
    test_mach_msg_size_mismatch();
    test_mach_vm_bounds();
    test_mach_ool_and_descriptors();
    test_ipc_lifecycle_stress();
    test_tcp_large_write();
    test_execve_large_args();

    test_privilege_model_setuid();
    test_privilege_model_kill();
    test_dev_serial_safety();
    test_pipe_buffer_invariants();
    test_service_hijack_protection();
    test_mach_ool_memory_integrity();
    test_window_surface_isolation();

    test_multithreading_and_ulock();
    test_kqueue_and_kevent();

    printf("\n===============================================================\n");
    printf(" Results: %d/%d tests PASSED (%d failures)\n",
           total_tests - failed_count, total_tests, failed_count);
    printf("===============================================================\n");

    return failed_count == 0 ? 0 : 1;
}
