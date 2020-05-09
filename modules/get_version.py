#!/usr/bin/env python3
import struct
import sys

def get_lk_version(filename):
    with open(filename, "rb") as lk:
        lk.seek(4)
        size = struct.unpack("<I", lk.read(4))
        lk.seek(size[0] + 0x200 - 0x102)
        return struct.unpack(">H", lk.read(2))[0]

def get_tz_version(filename):
    with open(filename, "rb") as tz:
        tz.seek(0x20f)
        return struct.unpack(">H", tz.read(2))[0]

def get_pl_version(filename):
    with open(filename, "rb") as pl:
        data = pl.read()
        offset = data.find(b"\x34\xb6\x12\x00\xbc\xbf\x12\x00")
        if offset:
            version = int.from_bytes(data[offset + 8: offset + 9], "little")
        else:
            version = 0xFF
        return version

def main():
    if len(sys.argv) != 3:
        print("Usage: " + sys.argv[0] + " [lk|pl|tz] <filename>")
        exit(1)
    else:
        if sys.argv[1] == "lk":
            print(get_lk_version(sys.argv[2]))
        if sys.argv[1] == "tz":
            print(get_tz_version(sys.argv[2]))
        if sys.argv[1] == "pl":
            print(get_pl_version(sys.argv[2]))

if __name__ == "__main__":
    main()
