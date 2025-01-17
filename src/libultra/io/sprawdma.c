#include "global.h"

s32 __osSpRawStartDma(s32 direction, void* devAddr, void* dramAddr, size_t size) {
    if (__osSpDeviceBusy() != 0) {
        return -1;
    }

    *(vu32*)0xA4040000 = devAddr;
    *(vu32*)0xA4040004 = osVirtualToPhysical(dramAddr);

    if (direction == 0) {
        *(vu32*)0xA404000C = size - 1;
    } else {
        *(vu32*)0xA4040008 = size - 1;
    }

    return 0;
}
