#!/bin/bash

set -e

. functions.inc

adb wait-for-device

max_tee=263
max_lk=2
max_pl=5

check_device "sloane" " - Amazon Fire TV (2nd generation) - "

get_root

pull_images

tee_version=$((`python3 modules/get_version.py tz dumps/tz_stock.bin | dos2unix`))
lk_version=$((`python3 modules/get_version.py lk dumps/lk_stock.bin | dos2unix`))
pl_version=$((`python3 modules/get_version.py pl dumps/pl_stock.bin | dos2unix`))

echo "PL version: ${pl_version} (${max_pl})"
echo "LK version: ${lk_version} (${max_lk})"
echo "TZ version: ${tee_version} (${max_tee})"
echo ""

if [ "$1" = "brick" ] || [ $tee_version -gt $max_tee ] || [ $lk_version -gt $max_lk ] || [ $pl_version -gt $max_pl ] ; then
  echo "TZ, Preloader or LK are too new, RPMB downgrade necessary (or brick option used)"
  echo "Brick preloader to continue via bootrom-exploit? (Type \"YES\" to continue)"
  read YES
  if [ "$YES" = "YES" ]; then
    echo "Bricking preloader"
    adb shell su -c \"echo 0 \> /sys/block/mmcblk0boot0/force_ro\"
    adb shell su -c \"dd if=/dev/zero of=/dev/block/mmcblk0boot0 bs=512 count=8\"
    adb shell su -c \"echo -n EMMC_BOOT \> /dev/block/mmcblk0boot0\"

    echo "Flashing LK"
    adb push bin/lk.bin /data/local/tmp/
    adb shell su -c \"dd if=/data/local/tmp/lk.bin of=/dev/block/platform/mtk-msdc.0/by-name/lk bs=512\" 
    echo ""

    echo "Flashing TZ"
    adb push bin/tz.img /data/local/tmp/
    adb shell su -c \"dd if=/data/local/tmp/tz.img of=/dev/block/platform/mtk-msdc.0/by-name/TEE1 bs=512\" 
    adb shell su -c \"dd if=/data/local/tmp/tz.img of=/dev/block/platform/mtk-msdc.0/by-name/TEE2 bs=512\" 
    echo ""

    echo "Rebooting..., continue with bootrom-step-minimal.sh"
    adb shell reboot
    exit 0
  fi
  exit 1
fi

echo "Your device will be reset to factory defaults..."
echo "Press Enter to Continue..."
read

echo "Dumping GPT"
[ ! -d gpt ] && mkdir gpt
adb shell su -c \"dd if=/dev/block/mmcblk0 bs=512 count=34 of=/data/local/tmp/gpt.bin\" 
adb shell su -c \"chmod 644 /data/local/tmp/gpt.bin\" 
adb pull /data/local/tmp/gpt.bin gpt/gpt.bin
echo ""

echo "Modifying GPT"
modules/gpt.py patch gpt/gpt.bin
echo ""

echo "Flashing temp GPT"
adb push gpt/gpt.bin.step1.gpt /data/local/tmp/
adb shell su -c \"dd if=/data/local/tmp/gpt.bin.step1.gpt of=/dev/block/mmcblk0 bs=512 count=34\" 
echo ""

echo "Preparing for Factory Reset"
adb shell su -c \"mkdir -p /cache/recovery\"
adb shell su -c \"echo --wipe_data \> /cache/recovery/command\"
adb shell su -c \"echo --wipe_cache \>\> /cache/recovery/command\"
echo ""

echo "Rebooting into Recovery"
adb reboot recovery
