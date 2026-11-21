"""Write an ESP-IDF otadata image that boots ota_<slot>. usage: make_otadata.py <slot> out.bin

Entry: u32 ota_seq, 20-byte label (0xFF), u32 ota_state, u32 crc where
crc = esp_rom_crc32_le(UINT32_MAX, &ota_seq, 4) == zlib.crc32(seq, 0xFFFFFFFF).
The bootloader boots ota[(seq - 1) % 2] from the valid entry with the highest seq.
"""
import struct, sys, zlib

slot = int(sys.argv[1])
seq = slot + 1
seq_bytes = struct.pack("<I", seq)
entry = seq_bytes + b"\xff" * 20 + struct.pack("<I", 0xFFFFFFFF) + struct.pack("<I", zlib.crc32(seq_bytes, 0xFFFFFFFF))
image = entry + b"\xff" * (0x1000 - len(entry)) + b"\xff" * 0x1000
open(sys.argv[2], "wb").write(image)
print("otadata for ota_%d, seq %d" % (slot, seq))
