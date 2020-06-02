#include <stdint.h>
#include "core.h"
#include "mmc.h"
#include "samsung_mmc.h"

unsigned int msdc_cmd(struct msdc_host *host, struct mmc_command *cmd);

int mmc_enter_read_ram(struct msdc_host *host) {
    int err;
    struct mmc_command cmd = { 0 };

    cmd.opcode = 62;
    cmd.arg = 0xefac62ec;
    cmd.flags = MMC_RSP_R1B; //29

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    cmd.opcode = 62;
    cmd.arg = 0x10210002;
    cmd.flags = MMC_RSP_R1B;

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    return 0;
}

int mmc_exit_cmd62(struct msdc_host *host) {
    int err;
    struct mmc_command cmd = { 0 };

    cmd.opcode = 62;
    cmd.arg = 0xefac62ec;
    cmd.flags = MMC_RSP_R1B;

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    cmd.opcode = 62;
    cmd.arg = 0xdeccee;
    cmd.flags = MMC_RSP_R1B;

    err = msdc_cmd(host, &cmd);
    if (err)
        return err;

    return 0;
}

int _mmc_read_mem(struct msdc_host *host, int addr, int size, void *buf) {
    int err;
    int i;
    struct mmc_command cmd = { 0 };

    for(i = 0; i < size; ++i) {
        cmd.opcode = MMC_ERASE_GROUP_START;
        cmd.flags = MMC_RSP_R1; //21
        cmd.arg = addr + i*BLOCK_SIZE;
        err = msdc_cmd(host, &cmd);
        if (err)
            return err;

        cmd.opcode = MMC_ERASE_GROUP_END;
        cmd.flags = MMC_RSP_R1;
        cmd.arg = BLOCK_SIZE;
        err = msdc_cmd(host, &cmd);
        if (err)
            return err;

        cmd.opcode = MMC_ERASE;
        cmd.flags = MMC_RSP_R1B;
        cmd.arg = 0;
        err = msdc_cmd(host, &cmd);
        if (err)
            return err;

        err = mmc_read(host, i, buf + i*BLOCK_SIZE);
        if (err)
            return err;
    }
    
    return 0;
}

int mmc_read_mem(struct msdc_host *host, int addr, void *buf) {
	return _mmc_read_mem(host, addr, BLOCK_SIZE, buf);
}