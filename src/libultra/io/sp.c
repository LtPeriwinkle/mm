#include "global.h"

s32 __osSpDeviceBusy(void) {
    register u32 status = HW_REG(SP_STATUS_REG, u32);

    if (status & (SP_STATUS_DMA_BUSY | SP_STATUS_DMA_FULL | SP_STATUS_IO_FULL)) {
        return true;
    }
    return false;
}
