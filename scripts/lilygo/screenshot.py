"""Save the T5's current screen as a PNG over USB serial.

usage: python screenshot.py COM7 out.png
Needs firmware with CMD:SCREENSHOT (all CrossMux builds) plus pyserial and Pillow.
The T5 framebuffer is 960x540 landscape; it is rotated to the 540x960 portrait view.
"""
import re
import sys
import time

import serial
from PIL import Image


def grab(port):
    s = serial.Serial(port, 115200, timeout=0.2)
    time.sleep(0.3)
    s.reset_input_buffer()
    s.write(b"CMD:SCREENSHOT\n")
    s.flush()
    buf, size, end = b"", None, time.time() + 15
    while time.time() < end:
        buf += s.read(65536)
        if size is None:
            m = re.search(rb"SCREENSHOT_START:(\d+)\r?\n", buf)
            if m:
                size, buf = int(m.group(1)), buf[m.end():]
        if size is not None and len(buf) >= size:
            s.close()
            return buf[:size]
    s.close()
    raise RuntimeError(f"screenshot timed out ({len(buf)} bytes)")


if __name__ == "__main__":
    raw = grab(sys.argv[1])
    Image.frombytes("1", (960, 540), raw).rotate(-90, expand=True).save(sys.argv[2])
    print("saved", sys.argv[2])
