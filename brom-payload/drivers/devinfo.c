#include "devinfo.h"
#include "mt_sd.h"

const uint32_t devinfo_addrs[] = { 
            0x10206020,
            0x10206030, 
            0x10206038,
            0x10206040,
            0x10206044,
            0x10206274,
            0x10206060,
            0x10206100,
            0x10206104,
            0x10206108,
            0x10206120,
            0x10206130,
            0x10206140,
            0x10206144,
            0x10206170,
            0x10206174,
            0x10206178,
            0x1020617C,
            0x10206180,
            0x10206184,
            0x8000000,
            0x10206188,
            0x10206504,
            0x10206514,
            0x10206700,
            0x10206704,
            0x10206708,
            0x1020670C,
            0x10206528,
            0x1020652C,
            0x10206530,
            0x10206534,
            0x10206538,
            0x10206540,
            0x10206544,
            0x10206548,
            0x102064C4,
            0x102064C8,
            0x102064B0,
            0x102064B8,
            0x10206090,
            0x10206094,
            0x10206098,
            0x1020609C,
            0x102060A0,
            0x102060A4,
            0x102060A8,
            0x102060AC
        };


uint32_t get_devinfo_with_index(uint32_t index) {
    if (index < 48) {
        //return (*(volatile unsigned int *)(devinfo_addrs[index]));
        return sdr_read32(devinfo_addrs[index]);
    }
    return -1;
}
