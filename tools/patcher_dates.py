"""Sets the dates in a rulebook's MacBinary (.bin) and HFS disk image (.dsk) to its release date, so that two
builds of the same source give the same files. Retro68's Rez stamps both with the time of the build; this
is the only thing that differs between builds.

    python3 tools/patcher_dates.py 2026-11-22 build/checkers-rulebook.bin build/checkers-rulebook.dsk

In the .bin: the creation and modification dates of the MacBinary header, and its CRC (MacBinary II). In the
.dsk: every 4-byte field that holds one of the build's times (the volume's dates in both master directory
blocks, and the folder's and the file's dates in the catalog), found by value: the times are those of the
.bin and of the volume's master directory block.
"""
import binascii
import datetime
import struct
import sys

MAC_EPOCH = datetime.datetime(1904, 1, 1)


def mac_time(day):
    """'2026-11-22' -> seconds since 1904, at noon (local time on the Mac is not known; noon keeps the date)."""
    d = datetime.datetime.strptime(day, "%Y-%m-%d").replace(hour=12)
    return int((d - MAC_EPOCH).total_seconds())


def fix_bin(blob, when):
    blob = bytearray(blob)
    old = {struct.unpack_from(">I", blob, 91)[0], struct.unpack_from(">I", blob, 95)[0]}
    struct.pack_into(">II", blob, 91, when, when)
    struct.pack_into(">H", blob, 124, binascii.crc_hqx(bytes(blob[:124]), 0))
    return bytes(blob), old


def fix_dsk(blob, when, old):
    blob = bytearray(blob)
    if blob[1024:1026] != b"BD":
        raise ValueError("not an HFS disk image")
    old = set(old) | {struct.unpack_from(">I", blob, 1026)[0], struct.unpack_from(">I", blob, 1030)[0]}
    new = struct.pack(">I", when)
    for t in old:
        pat = struct.pack(">I", t)
        i = blob.find(pat)
        while i >= 0:
            blob[i:i + 4] = new
            i = blob.find(pat, i + 4)
    return bytes(blob)


def main(argv):
    day, bin_path, dsk_path = argv
    when = mac_time(day)
    with open(bin_path, "rb") as f:
        mb, old = fix_bin(f.read(), when)
    with open(dsk_path, "rb") as f:
        dsk = fix_dsk(f.read(), when, old)
    with open(bin_path, "wb") as f:
        f.write(mb)
    with open(dsk_path, "wb") as f:
        f.write(dsk)


if __name__ == "__main__":
    main(sys.argv[1:])
