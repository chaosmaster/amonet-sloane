#include <inttypes.h>

#include "libc.h"

#include "drivers/types.h"
#include "drivers/core.h"
#include "drivers/mt_sd.h"
#include "drivers/errno.h"
#include "drivers/mmc.h"
#include "drivers/samsung_mmc.h"
#include "drivers/timer.h"

void low_uart_put(int ch) {
    volatile uint32_t *uart_reg0 = (volatile uint32_t*)0x11002014;
    volatile uint32_t *uart_reg1 = (volatile uint32_t*)0x11002000;

    while ( !((*uart_reg0) & 0x20) )
    {}

    *uart_reg1 = ch;
}

void _putchar(char character)
{
    if (character == '\n')
        low_uart_put('\r');
    low_uart_put(character);
}

void hex_dump(const void* data, size_t size) {
    size_t i, j;
    for (i = 0; i < size; ++i) {
        printf("%02X ", ((unsigned char*)data)[i]);
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("\n");
            } else if (i+1 == size) {
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("\n");
            }
        }
    }
}

void sleepy(void) {
    // TODO: do better
    for (volatile int i = 0; i < 0x80000; ++i) {}
}

/*
void mdelay (unsigned long msec)
{
    (void)msec;
    sleepy();
}
*/


/* delay usec useconds */
/*
void udelay (unsigned long usec)
{
    (void)usec;
    sleepy();
}
*/

int main() {
    char buf[0x200] = { 0 };
    int ret = 0;

    int (*send_dword)() = (void*)0x9FFF;
    int (*recv_dword)() = (void*)0x9FCB;
    // addr, sz
    int (*send_data)() = (void*)0xA0C7;
    // addr, sz, flags (=0)
    int (*recv_data)() = (void*)0xA041;

    // Restore the pointer we overwrote
    uint32_t *ptr_send = (void*)0x1227B4;
    *ptr_send = 0x3ABF;

    printf("Entered the payload\n");

    struct msdc_host host = { 0 };
    host.ocr_avail = MSDC_OCR_AVAIL;

    printf("Entering command loop\n");

    send_dword(0xB1B2B3B4);

    mmc_init(&host);

    while (1) {
        memset(buf, 0, sizeof(buf));
        uint32_t magic = recv_dword();
        if (magic != 0xf00dd00d) {
            printf("Protocol error\n");
            printf("Magic received = 0x%08X\n", magic);
            break;
        }
        uint32_t cmd = recv_dword();
        switch (cmd) {
        case 0x1000: {
            uint32_t block = recv_dword();
            printf("Read block 0x%08X\n", block);
            memset(buf, 0, sizeof(buf));
            if (mmc_read(&host, block, buf) != 0) {
                printf("Read error!\n");
            } else {
                send_data(buf, sizeof(buf));
            }
            break;
        }
        case 0x1001: {
            uint32_t block = recv_dword();
            printf("Write block 0x%08X ", block);
            memset(buf, 0, sizeof(buf));
            recv_data(buf, 0x200, 0);
            if (mmc_write(&host, block, buf) != 0) {
                printf("Write error!\n");
            } else {
                printf("OK\n");
                send_dword(0xD0D0D0D0);
            }
            break;
        }
        case 0x1002: {
            uint32_t part = recv_dword();
            printf("Switch to partition %d => ", part);
            ret = mmc_set_part(&host, part);
            printf("0x%08X\n", ret);
            mdelay(500); // just in case
            break;
        }
        case 0x2000: {
            printf("Read rpmb\n");
            mmc_rpmb_read(&host, buf);
            send_data(buf, 0x100);
            break;
        }
        case 0x2001: {
            printf("Write rpmb\n");
            recv_data(buf, 0x100, 0);
            mmc_rpmb_write(&host, buf);
            break;
        }
        case 0x3000: {
            printf("Reboot\n");
            volatile uint32_t *reg = (volatile uint32_t *)0x10007000;
            reg[8/4] = 0x1971;
            reg[0/4] = 0x22000014;
            reg[0x14/4] = 0x1209;

            while (1) {

            }
        }
        case 0x3001: {
            printf("Kick watchdog\n");
            volatile uint32_t *reg = (volatile uint32_t *)0x10007000;
            reg[8/4] = 0x1971;
            break;
        }
        case 0x6000 : {
            printf("Reading emmc cid\n");
            send_data(mmc_get_cid(), 0x10);
            break;
        }
        case 0x6001 : {
            printf("Reading emmc csd\n");
	    //mmc_read_csd(&host, 0, (u32*)buf);
            // CSD should be 128 bit
            send_data(mmc_get_csd(), 0x10);
            break;
        }
        case 0x6002 : {
            printf("Reading emmc ext_csd\n");
	    mmc_read_ext_csd(&host,(u8*)buf);
            // CSD should be 128 bit
            send_data(buf, 512);
            break;
        }
        case 0x7000 : {
            printf("Enter samsung backdoor\n");
	    int res = mmc_enter_read_ram(&host);
	    printf("%d\n", res);
	    send_dword(res);
            break;
        }
        case 0x7001 : {
            printf("Exit samsung backdoor\n");
	    send_dword(mmc_exit_cmd62(&host));
            break;
	}
        case 0x7002 : {
            uint32_t block = recv_dword();
            printf("Read block 0x%08X\n", block);
            memset(buf, 0, sizeof(buf));
            if (mmc_read_mem(&host, block * 512, buf) != 0) {
                printf("Read error!\n");
            } else {
                send_data(buf, sizeof(buf));
            }
            break;
        }
        case 0x7003 : {
            printf("Getting prep result\n");
            send_dword(mmc_get_prep_result());
            break;
        }
        default:
            printf("Invalid command\n");
            break;
        }
    }

    printf("Exiting the payload\n");

    while (1) {

    }
}
