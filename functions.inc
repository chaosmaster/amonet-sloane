function get_root() {
  set +e
  echo "Testing root access..."
  adb shell su -c id | grep uid=0\(root\)
  ROOT=$?
  if [ $ROOT -ne 0 ]; then
    echo ""
    echo "Trying to use mtk-su to get temp root..."
    check_su
    push_root
    while [ $ROOT -ne 0 ]; do
      adb shell /data/local/tmp/mtk-su -c "/data/local/tmp/busybox mount /data/local/tmp/minisu.img /system/xbin/"
      adb shell su -c id | grep uid=0\(root\)
      ROOT=$?
      [ $ROOT -ne 0 ] && sleep 1
    done
  fi
  echo ""
  set -e
}

function check_su() {
  if [ ! -f "bin/mtk-su" ]; then
    echo "No mtk-su found, you can get it here: https://forum.xda-developers.com/hd8-hd10/orig-development/experimental-software-root-hd-8-hd-10-t3904595"
    exit 1
  fi
}

function pull_images() {
   [ ! -d dumps ] && mkdir dumps
   adb shell su -c \"dd if=/dev/block/platform/mtk-msdc.0/by-name/TEE1 of=/data/local/tmp/tz1_stock.bin bs=512\"
   adb shell su -c \"dd if=/dev/block/platform/mtk-msdc.0/by-name/TEE2 of=/data/local/tmp/tz2_stock.bin bs=512\"
   adb shell su -c \"dd if=/dev/block/platform/mtk-msdc.0/by-name/lk of=/data/local/tmp/lk_stock.bin bs=512\"
   adb shell su -c \"dd if=/dev/block/mmcblk0boot0 of=/data/local/tmp/pl_stock.bin bs=512\"
   adb shell su -c \"chmod 644 /data/local/tmp/lk_stock.bin\"
   adb shell su -c \"chmod 644 /data/local/tmp/tz1_stock.bin\"
   adb shell su -c \"chmod 644 /data/local/tmp/tz2_stock.bin\"
   adb shell su -c \"chmod 644 /data/local/tmp/pl_stock.bin\" 
   adb pull /data/local/tmp/lk_stock.bin dumps/
   adb pull /data/local/tmp/tz1_stock.bin dumps/
   adb pull /data/local/tmp/tz2_stock.bin dumps/
   adb pull /data/local/tmp/pl_stock.bin dumps/
}

function push_root() {
  echo "Pushing root files"
  adb push bin/mtk-su /data/local/tmp/
  adb shell chmod 755 /data/local/tmp/mtk-su
  adb push bin/minisu.img /data/local/tmp/
  adb push bin/busybox /data/local/tmp/
  adb shell chmod 755 /data/local/tmp/busybox
}

function check_device() {
  product=$(adb shell getprop ro.build.product | dos2unix)

  if [ "$product" != "${1}" ] ; then
    echo "This is only for the \"${1}\"${2}, your device is a \"${product}\""
    exit 1
  fi
}

function get_lk_version() {
	LK=$1
	if [ "$(dd if=${LK} bs=1 skip=8 count=2 2>/dev/null)" == "LK" ]; then
		size=$(dd if=${LK} bs=1 skip=4 count=4 2>/dev/null| od -bI | awk 'NR==2')
		offset=$(($size + 0x200 - 0x102))
		version=0x$(dd if=${LK} bs=1 skip=$offset count=2 2>/dev/null| od -bx | awk 'NR==2' | awk '{$1=$1};1')
		version=$(( (version<<8 &0xff00) | (version>>8 & 0x00ff) ))
	else
		version=$((0xFFFF))
	fi
	echo $version
}

function get_tz_version() {
	TZ=$1
	if [ "$(dd if=${TZ} bs=1 skip=8 count=3 2>/dev/null)" == "ATF" ]; then
		version=0x$(dd if=${TZ} bs=1 skip=$((0x20f)) count=2 2>/dev/null| od -bx | awk 'NR==2' | awk '{$1=$1};1')
		version=$(( (version<<8 &0xff00) | (version>>8 & 0x00ff) ))
	else
		version=$((0xFFFF))
	fi
	echo $version
}

function get_pl_version() {
	PL=$1
	hexdump -ve '1/1 ":%.2x"' $PL| grep "34:b6:12:00:bc:bf:12:00" >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		version=$((0x$(hexdump -ve '1/1 ":%.2x"' $PL| sed "s/34:b6:12:00:bc:bf:12:00:\([0-9a-f]\{2\}\).*/\1/g" | awk -F ":" '{print $NF}')))
	else
		version=$((0xFFFF))
	fi
	echo $version
}
