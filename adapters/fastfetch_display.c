#include "detection/displayserver/displayserver.h"
#include <chimera_telemetry.h>
#include <kernel/process_stats.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>

void ffConnectDisplayServerImpl(FFDisplayServerResult *result) {
    static chimera_process_stats_t processes[64];
    long count=syscall(SYS_proclist,(long)processes,64L,
                      (long)CHIMERA_PROCESS_STATS_V1,(long)sizeof(processes[0]),0UL,0UL);
    for(long i=0;i<count;++i) {
        const char *base=strrchr(processes[i].executable,'/');
        base=base?base+1:processes[i].executable;
        if(!strcmp(base,"canvas")) {
            ffStrbufSetS(&result->wmProcessName,base);
            ffStrbufSetS(&result->wmPrettyName,base);break;
        }
    }
    chimera_display_info_t display;
    if(syscall(SYS_sysinfo,(long)&display,(long)CHIMERA_SYSINFO_DISPLAY,0UL,
               (long)sizeof(display),0UL,0UL) || display.version!=CHIMERA_DISPLAY_INFO_VERSION)return;
    const char *backend=display.source==CHIMERA_DISPLAY_KMS?"DRM/KMS":"Framebuffer";
    FFstrbuf name;ffStrbufInitS(&name,backend);
    ffdsAppendDisplay(result,display.width,display.height,display.refresh_millihz/1000.0,
                      0,0,0,0.0,0,&name,FF_DISPLAY_TYPE_UNKNOWN,true,0,0,0,backend);
}
