"""Capture Home on the T5 for every CrossInk theme over USB serial.

usage: theme_shots.py COM7 out_dir [suffix]
Needs firmware with CMD:THEME:<n> (in-memory theme switch) and CMD:SCREENSHOT.
"""
import re, sys, time
import serial
from PIL import Image

THEMES = ["classic", "lyra", "lyra3", "roundedraff", "carousel", "minimal", "dashboard", "covergrid"]

def screenshot(s):
    s.reset_input_buffer()
    s.write(b"CMD:SCREENSHOT\n"); s.flush()
    buf, size, end = b"", None, time.time() + 15
    while time.time() < end:
        buf += s.read(65536)
        if size is None:
            m = re.search(rb"SCREENSHOT_START:(\d+)\r?\n", buf)
            if m:
                size, buf = int(m.group(1)), buf[m.end():]
        if size is not None and len(buf) >= size:
            return buf[:size]
    raise RuntimeError(f"screenshot timed out ({len(buf)} bytes)")

def main():
    port, out = sys.argv[1], sys.argv[2]
    suffix = sys.argv[3] if len(sys.argv) > 3 else ""
    s = serial.Serial(port, 115200, timeout=0.2)
    time.sleep(0.3)
    for i, name in enumerate(THEMES):
        s.reset_input_buffer()
        s.write(f"CMD:THEME:{i}\n".encode()); s.flush()
        ok, end, buf = False, time.time() + 5, b""
        while time.time() < end and not ok:
            buf += s.read(4096)
            ok = f"THEME_OK:{i}".encode() in buf
        if not ok:
            print(f"{name}: no THEME_OK"); continue
        time.sleep(7)  # Home render + cover thumbnail generation + e-ink refresh
        raw = screenshot(s)
        path = f"{out}/theme_{i}_{name}{suffix}.png"
        Image.frombytes("1", (960, 540), raw).rotate(-90, expand=True).save(path)
        print("saved", path)
    s.close()

main()
