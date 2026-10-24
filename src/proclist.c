#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

typedef struct {
  unsigned int pid;
  unsigned int ppid;
  unsigned int state;
  unsigned int thread_count;
  char name[32];
} procinfo_t;

#define SYS_proclist 252
#define MAX_PROCS 64

#define PROC_STATE_RUNNING  1
#define PROC_STATE_EXITED   2

static void write_str(const char *s) {
    int len = 0;
    while (s[len]) len++;
    write(1, s, len);
}

static void write_num(unsigned int n) {
    char buf[12];
    int i = 0;
    if (n == 0) {
        write(1, "0", 1);
        return;
    }
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i > 0) {
        write(1, &buf[--i], 1);
    }
}

int main(void) {
    procinfo_t procs[MAX_PROCS];
    
    long count = syscall(SYS_proclist, (long)procs, (long)MAX_PROCS);
    
    if (count < 0) {
        write_str("Error: proclist syscall failed\n");
        return 1;
    }
    
    write_str("\033[1;37mPID   PPID  STATE    THREADS  NAME\n");
    write_str("===   ====  =======  =======  ====\033[0m\n");
    
    for (long i = 0; i < count; i++) {
        if (procs[i].state == PROC_STATE_RUNNING) {
            write_str("\033[1;32m");
        } else {
            write_str("\033[1;31m");
        }
        
        write_num(procs[i].pid);
        write_str("     ");
        
        write_num(procs[i].ppid);
        write_str("     ");
        
        if (procs[i].state == PROC_STATE_RUNNING) {
            write_str("Running");
        } else {
            write_str("Exited ");
        }
        write_str("  ");
        
        write_num(procs[i].thread_count);
        write_str("        ");
        
        write_str(procs[i].name);
        write_str("\033[0m\n");
    }
    
    write_str("\nTotal processes: ");
    write_num((unsigned int)count);
    write_str("\n");
    
    return 0;
}
