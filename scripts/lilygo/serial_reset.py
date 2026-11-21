"""Pulse a normal (run-from-flash) reset on the T5 over its USB-Serial/JTAG port."""
import sys
import time

import serial

s = serial.Serial(sys.argv[1], 115200, timeout=0.1)
s.dtr = False
s.rts = True
time.sleep(0.1)
s.rts = False
s.close()
print("reset sent")
