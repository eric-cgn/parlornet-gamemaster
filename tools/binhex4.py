"""BinHex 4.0 (.hqx) from a MacBinary file, and back, for the release files (tools/build.sh).

Python dropped its binhex module (3.11), so this follows the BinHex 4.0 format itself: a header
(name, type, creator, Finder flags, fork lengths) and the two forks, each part followed by its
CRC-16 (CCITT, as binascii.crc_hqx), run-length encoded (0x90 marks a run) and written six bits
to a character, 64 characters to a line, between colons.

    python3 tools/binhex4.py "Eclipse 68k Fix.bin" eclipse-68k-fix.hqx
"""
import binascii
import struct
import sys

ALPHABET = b"!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr"
BANNER = b"(This file must be converted with BinHex 4.0)"
RUN = 0x90


def read_macbinary(blob):
    """MacBinary (I or II) -> dict with name, type, creator, flags, data, rsrc."""
    if len(blob) < 128 or blob[0] != 0 or not 1 <= blob[1] <= 63:
        raise ValueError("not a MacBinary file")
    name = blob[2:2 + blob[1]]
    dlen, rlen = struct.unpack(">II", blob[83:91])
    flags = blob[73] << 8 | blob[101]
    data_at = 128
    rsrc_at = data_at + (dlen + 127) // 128 * 128
    return {"name": name, "type": blob[65:69], "creator": blob[69:73], "flags": flags,
            "data": blob[data_at:data_at + dlen], "rsrc": blob[rsrc_at:rsrc_at + rlen]}


def _with_crc(part):
    return part + struct.pack(">H", binascii.crc_hqx(part, 0))


def _rle(raw):
    out, i = bytearray(), 0
    while i < len(raw):
        c = raw[i]
        n = 1
        while i + n < len(raw) and raw[i + n] == c and n < 255:
            n += 1
        lit = bytes([c, 0]) if c == RUN else bytes([c])
        if n >= 4:  # c RUN n: c repeated n times (three are as short written out)
            out += lit + bytes([RUN, n])
        else:
            out += lit * n
        i += n
    return bytes(out)


def encode(info):
    """info as from read_macbinary -> the .hqx file's bytes (CR line ends, as on a Mac)."""
    name = info["name"]
    head = (bytes([len(name)]) + name + b"\0" + info["type"] + info["creator"]
            + struct.pack(">HII", info["flags"], len(info["data"]), len(info["rsrc"])))
    raw = _with_crc(head) + _with_crc(info["data"]) + _with_crc(info["rsrc"])
    packed = _rle(raw)
    chars = bytearray()
    for i in range(0, len(packed), 3):
        chunk = packed[i:i + 3]
        bits = int.from_bytes(chunk.ljust(3, b"\0"), "big")
        chars += bytes(ALPHABET[bits >> s & 63] for s in (18, 12, 6, 0))[:len(chunk) + 1]
    text = b":" + bytes(chars) + b":"
    lines = [text[i:i + 64] for i in range(0, len(text), 64)]
    return BANNER + b"\r\r" + b"\r".join(lines) + b"\r"


def decode(hqx):
    """The .hqx file's bytes -> dict with name, type, creator, flags, data, rsrc (CRCs checked)."""
    start = hqx.index(BANNER) + len(BANNER)
    start = hqx.index(b":", start) + 1
    body = bytes(ch for ch in hqx[start:hqx.index(b":", start)] if ch not in b" \t\r\n")
    index = {ch: i for i, ch in enumerate(ALPHABET)}
    packed = bytearray()
    for i in range(0, len(body), 4):
        group = body[i:i + 4]
        bits = 0
        for ch in group.ljust(4, ALPHABET[:1]):
            bits = bits << 6 | index[ch]
        packed += bits.to_bytes(3, "big")[:len(group) - 1]
    raw, i, last = bytearray(), 0, None
    while i < len(packed):
        c = packed[i]
        if c == RUN:
            n = packed[i + 1]
            i += 2
            if n == 0:
                raw.append(RUN)
                last = RUN
            else:
                raw += bytes([last]) * (n - 1)
            continue
        raw.append(c)
        last = c
        i += 1
    raw = bytes(raw)

    def part(at, length):
        chunk, crc = raw[at:at + length], raw[at + length:at + length + 2]
        if struct.pack(">H", binascii.crc_hqx(chunk, 0)) != crc:
            raise ValueError("CRC mismatch")
        return chunk, at + length + 2
    n = raw[0]
    head, at = part(0, 1 + n + 1 + 4 + 4 + 2 + 4 + 4)
    flags, dlen, rlen = struct.unpack(">HII", head[-10:])
    data, at = part(at, dlen)
    rsrc, at = part(at, rlen)
    return {"name": head[1:1 + n], "type": head[2 + n:6 + n], "creator": head[6 + n:10 + n], "flags": flags,
            "data": data, "rsrc": rsrc}


def main(argv):
    src, dst = argv
    with open(src, "rb") as f:
        info = read_macbinary(f.read())
    out = encode(info)
    if decode(out) != info:
        raise SystemExit("round trip failed")
    with open(dst, "wb") as f:
        f.write(out)


if __name__ == "__main__":
    main(sys.argv[1:])
