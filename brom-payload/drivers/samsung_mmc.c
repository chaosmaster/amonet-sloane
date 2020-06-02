#include <stdint.h>
#include "core.h"
#include "mmc.h"
#include "mt_sd.h"
#include "samsung_mmc.h"

unsigned int msdc_cmd(struct msdc_host *host, struct mmc_command *cmd);
void sleepy(void);

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

#define GPIO_BASE (0x10005000)
#define MSDC0_GPIO_RST_BASE                   (GPIO_BASE + 0xD00)
#define MSDC0_GPIO_DS_BASE                    (GPIO_BASE + 0xD10)

#define MSDC_GPIO_PULL_UP        (0)
#define MSDC_GPIO_PULL_DOWN      (1)

#define GPIO_CLK_CTRL       (0)
#define GPIO_CMD_CTRL       (1)
#define GPIO_DAT_CTRL       (2)
#define GPIO_RST_CTRL       (3)
#define GPIO_DS_CTRL        (4)
#define GPIO_MODE_CTRL      (5)

#define MSDC_PULL_0K        (0)
#define MSDC_PULL_10K       (1)
#define MSDC_PULL_50K       (2)
#define MSDC_PULL_8K        (3)

#define GPIO_MSDC_R1R0_MASK                   (0x3UL  <<  0)
#define GPIO_MSDC_PUPD_MASK                   (0x1UL  <<  2)

unsigned int msdc_uffs(unsigned int x)
{
    unsigned int r = 1;

    if (!x)
        return 0;
    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

#define msdc_set_field(reg,field,val) \
                        do{ \
                        u32 tv = sdr_read32(reg); \
                        tv &= ~((u32)(field)); \
                        tv |= ((val) << (msdc_uffs((unsigned int)field) - 1)); \
                        sdr_write32(reg,tv); \
                }while(0)


#define MSDC_SET_FIELD(reg,field,val) msdc_set_field(reg,field,val)


static void msdc_pin_pud(u32 pin, u32 mode, u32 val)
{
    switch(pin){
        case GPIO_CLK_CTRL:
            MSDC_SET_FIELD(MSDC0_GPIO_CLK_BASE, GPIO_MSDC_R1R0_MASK, val);
            MSDC_SET_FIELD(MSDC0_GPIO_CLK_BASE, GPIO_MSDC_PUPD_MASK, mode);
            break;
        case GPIO_CMD_CTRL:
            MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_MSDC_R1R0_MASK, val);
            MSDC_SET_FIELD(MSDC0_GPIO_CMD_BASE, GPIO_MSDC_PUPD_MASK, mode);
            break;
        case GPIO_DAT_CTRL:
            MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_MSDC_R1R0_MASK, val);
            MSDC_SET_FIELD(MSDC0_GPIO_DAT_BASE, GPIO_MSDC_PUPD_MASK, mode);
            break;
        case GPIO_RST_CTRL:
            MSDC_SET_FIELD(MSDC0_GPIO_RST_BASE, GPIO_MSDC_R1R0_MASK, val);
            MSDC_SET_FIELD(MSDC0_GPIO_RST_BASE, GPIO_MSDC_PUPD_MASK, mode);
            break;
        case GPIO_DS_CTRL:
            MSDC_SET_FIELD(MSDC0_GPIO_DS_BASE, GPIO_MSDC_R1R0_MASK, val);
            MSDC_SET_FIELD(MSDC0_GPIO_DS_BASE, GPIO_MSDC_PUPD_MASK, mode);
            break;
        default:
            break;
    }
}

void msdc_config_pin(int mode)
{
    //MSG(CFG, "[SD%d] Pins mode(%d), none(0), down(1), up(2), keep(3)\n",
    //    host->id, mode);

    switch (mode) {
        case MSDC_PIN_PULL_UP:
            msdc_pin_pud(GPIO_CLK_CTRL, MSDC_GPIO_PULL_DOWN, MSDC_PULL_50K); //clock should pull down 50k before using
            msdc_pin_pud(GPIO_CMD_CTRL, MSDC_GPIO_PULL_UP, MSDC_PULL_10K);
            msdc_pin_pud(GPIO_DAT_CTRL, MSDC_GPIO_PULL_UP, MSDC_PULL_10K);
            msdc_pin_pud(GPIO_DS_CTRL, MSDC_GPIO_PULL_DOWN, MSDC_PULL_50K);
            break;
        case MSDC_PIN_PULL_DOWN:
            msdc_pin_pud(GPIO_CLK_CTRL, MSDC_GPIO_PULL_DOWN, MSDC_PULL_50K);
            msdc_pin_pud(GPIO_CMD_CTRL, MSDC_GPIO_PULL_DOWN, MSDC_PULL_50K);
            msdc_pin_pud(GPIO_DAT_CTRL, MSDC_GPIO_PULL_DOWN, MSDC_PULL_50K);
            msdc_pin_pud(GPIO_DS_CTRL, MSDC_GPIO_PULL_DOWN, MSDC_PULL_50K);
            break;
        case MSDC_PIN_PULL_NONE:
            msdc_pin_pud(GPIO_CMD_CTRL, MSDC_GPIO_PULL_DOWN, MSDC_PULL_0K);
            msdc_pin_pud(GPIO_DAT_CTRL, MSDC_GPIO_PULL_DOWN, MSDC_PULL_0K);
        default:
            break;
    }
}


void msdc_host_power(int on)
{
    //MSG(CFG, "[SD%d] Turn %s %s power \n", host->id, on ? "on" : "off", "host");

    if (on) {
        msdc_config_pin(MSDC_PIN_PULL_UP);
        //msdc_set_host_pwr(host->id, 1);
        //msdc_clock(host, 1);
    } else {
        //msdc_clock(host, 0);
        //msdc_set_host_pwr(host->id, 0);
        msdc_config_pin(MSDC_PIN_PULL_DOWN);
    }
}

int prepare_mmc(struct msdc_host *host, int bootrom)
{
	int ret = 0;
    	struct mmc_command cmd = { 0 };

	//emmc_poweroff();
	//sleep(2000);
	msdc_host_power(0);
	sleepy();

	//emmc_poweron();
	//s5c_mshc_init(mmc_dev);
	//clk1(mmc_dev, 1);
	//clk2(mmc_dev, 0);
	//sleep(10);
	msdc_host_power(1);
	sleepy();
	if (bootrom) {
		cmd.opcode = 1;
		cmd.flags = MMC_RSP_PRESENT;
		cmd.arg = 0x69FF87A9;
		ret = msdc_cmd(host, &cmd);
		//sleep(10);
		sleepy();
	} else {
		//sleep(1000);
		sleepy();
	}

	//if ((ret = mmc_send_op_cond(mmc_dev)) < 0) return ret;
	//if ((ret = mmc_startup(mmc_dev)) < 0) return ret;

	return ret;
}

