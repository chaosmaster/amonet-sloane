#!/bin/bash

set -e

echo "Press Enter to Continue..."
read

fastboot flash recovery_x bin/twrp.img
fastboot flash MISC bin/boot-recovery.bin
fastboot reboot

echo ""
echo ""
echo "Your device should now reboot into TWRP"
echo ""
