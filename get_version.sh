#!/sbin/sh

get_lk_version() {
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

get_tz_version() {
	TZ=$1
	if [ "$(dd if=${TZ} bs=1 skip=8 count=3 2>/dev/null)" == "ATF" ]; then
		version=0x$(dd if=${TZ} bs=1 skip=$((0x20f)) count=2 2>/dev/null| od -bx | awk 'NR==2' | awk '{$1=$1};1')
		version=$(( (version<<8 &0xff00) | (version>>8 & 0x00ff) ))
	else
		version=$((0xFFFF))
	fi
	echo $version
}

get_pl_version() {
	PL=$1
	hexdump -ve '1/1 ":%.2x"' $PL| grep "34:b6:12:00:bc:bf:12:00" >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		version=$((0x$(hexdump -ve '1/1 ":%.2x"' $PL| sed "s/34:b6:12:00:bc:bf:12:00:\([0-9a-f]\{2\}\).*/\1/g" | awk -F ":" '{print $NF}')))
	else
		version=$((0xFFFF))
	fi
	echo $version
}
