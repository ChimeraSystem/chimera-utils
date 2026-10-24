#include "detection/gpu/gpu.h"
#include <chimera_telemetry.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>

const char *ffDetectGPUImpl(const FFGPUOptions *options,FFlist *gpus) {
    FF_UNUSED(options)
    for(uint32_t index=0;index<256;++index) {
        chimera_gpu_info_t info;
        if(syscall(SYS_sysinfo,(long)&info,(long)CHIMERA_SYSINFO_GPU,(long)index,
                   (long)sizeof(info),0UL,0UL)) {
            if(errno==ENOENT)break;
            return "GPU inventory query failed";
        }
        if(info.version!=CHIMERA_GPU_INFO_VERSION)return "Unsupported GPU inventory version";
        FFGPUResult *gpu=FF_LIST_ADD(FFGPUResult,*gpus);
        const char *vendor=ffGPUGetVendorString(info.vendor_id);
        if(vendor)ffStrbufInitS(&gpu->vendor,vendor);
        else ffStrbufInitF(&gpu->vendor,"PCI %04x",info.vendor_id);
        ffStrbufInitF(&gpu->name,"PCI GPU %04x:%04x",info.vendor_id,info.device_id);
        ffStrbufInitS(&gpu->driver,info.driver);
        ffStrbufInitS(&gpu->platformApi,"PCI");ffStrbufInit(&gpu->memoryType);
        gpu->type=FF_GPU_TYPE_UNKNOWN;gpu->temperature=FF_GPU_TEMP_UNSET;
        gpu->coreUsage=FF_GPU_CORE_USAGE_UNSET;gpu->coreCount=FF_GPU_CORE_COUNT_UNSET;
        gpu->frequency=FF_GPU_FREQUENCY_UNSET;gpu->index=index;gpu->deviceId=info.device_id;
        gpu->pcieSpeed=0;gpu->dedicated.total=FF_GPU_VMEM_SIZE_UNSET;
        gpu->dedicated.used=FF_GPU_VMEM_SIZE_UNSET;gpu->shared.total=FF_GPU_VMEM_SIZE_UNSET;
        gpu->shared.used=FF_GPU_VMEM_SIZE_UNSET;
    }
    return gpus->length?nullptr:"No display-class PCI device found";
}
