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

        cmd.opcode = MMC_READ_SINGLE_BLOCK;
        cmd.flags = MMC_RSP_R1;
        cmd.arg = 0;
    	msdc_set_blknum(host, 1);
        err = msdc_cmd(host, &cmd);
        if (err)
            return err;

	return msdc_pio_read(host, buf);

        //err = mmc_read(host, i, buf + i*BLOCK_SIZE);
        //if (err)
        //    return err;
    }
    
    return 0;
}

int mmc_read_mem(struct msdc_host *host, int addr, void *buf) {
	return _mmc_read_mem(host, addr, BLOCK_SIZE, buf);
}

int prepare_mmc(struct msdc_host *host, int bootrom)
{
	int ret = 0;
    	struct mmc_command cmd = { 0 };

	//emmc_poweroff();
	//sleep(2000);

	//emmc_poweron();
	//s5c_mshc_init(mmc_dev);
	//clk1(mmc_dev, 1);
	//clk2(mmc_dev, 0);
	//sleep(10);
	if (bootrom) {
		cmd.opcode = 1;
		cmd.flags = MMC_RSP_PRESENT;
		cmd.arg = 0x69FF87A9;
		ret = msdc_cmd(host, &cmd);
		//sleep(10);
	} else {
		//sleep(1000);
	}

	//if ((ret = mmc_send_op_cond(mmc_dev)) < 0) return ret;
	//if ((ret = mmc_startup(mmc_dev)) < 0) return ret;

	return ret;
}
