"""Open every CMD:OPEN screen on the T5, screenshot each, and build a contact sheet.

usage: python screen_tour.py COM7 <out_dir>
Needs a CrossMux build with CMD:OPEN, CMD:SCREENSHOT; pip install pyserial pillow.
Reader views open the last book you read (APP_STATE.openEpubPath).
"""
import re, sys, time
import serial
from PIL import Image, ImageDraw

SCREENS = ["home", "controlcenter", "library", "files", "settings", "reader", "drawer0", "drawer1", "drawer2",
           "drawer3", "drawer4", "contents", "percent", "bookstats", "readeroptions", "controls", "bookmarks",
           "clippings", "lookuphistory", "statusbar"]
port, out = sys.argv[1], sys.argv[2]

def send_wait(s, cmd, token, timeout=15):
    s.reset_input_buffer(); s.write(cmd); s.flush()
    buf, end = b"", time.time() + timeout
    while time.time() < end:
        buf += s.read(4096)
        m = re.search(token, buf)
        if m: return m.group(0).decode()
    return None

def shot(s):
    s.reset_input_buffer(); s.write(b"CMD:SCREENSHOT\n"); s.flush()
    buf, size, end = b"", None, time.time() + 10
    while time.time() < end:
        buf += s.read(65536)
        if size is None:
            m = re.search(rb"SCREENSHOT_START:(\d+)\r?\n", buf)
            if m: size, buf = int(m.group(1)), buf[m.end():]
        if size is not None and len(buf) >= size: return buf[:size]
    return None

s = serial.Serial(port, 115200, timeout=0.1); time.sleep(0.3)
saved = []
for name in SCREENS:
    # Start each reader view from Home so screens never stack inside each other.
    send_wait(s, b"CMD:OPEN:home\n", rb"OPEN_(OK|ERR):home"); time.sleep(3)
    reply = send_wait(s, f"CMD:OPEN:{name}\n".encode(), rb"OPEN_(OK|ERR):[a-z0-9]+")
    print(name, "->", reply, flush=True)
    if not reply or "ERR" in reply: continue
    time.sleep(9 if name not in ("home", "settings", "files", "library", "plugins") else 5)
    raw = shot(s)
    if raw:
        path = f"{out}/{len(saved)+1:02d}_{name}.png"
        Image.frombytes("1", (960, 540), raw).rotate(-90, expand=True).save(path)
        saved.append((name, path)); print("  saved", path, flush=True)
send_wait(s, b"CMD:OPEN:home\n", rb"OPEN_(OK|ERR):home"); s.close()

# Contact sheet, 5 per row.
w, h, cols = 216, 384, 5
rows = (len(saved) + cols - 1) // cols
sheet = Image.new("L", (cols * (w + 8), rows * (h + 26)), 200); d = ImageDraw.Draw(sheet)
for i, (name, path) in enumerate(saved):
    x, y = (i % cols) * (w + 8), (i // cols) * (h + 26)
    d.text((x + 3, y + 4), f"{i+1} {name}", fill=0)
    sheet.paste(Image.open(path).convert("L").resize((w, h), Image.LANCZOS), (x, y + 20))
sheet.save(f"{out}/sheet.png"); print("TOUR DONE", len(saved), "screens", flush=True)
