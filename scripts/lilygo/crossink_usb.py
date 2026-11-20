"""Minimal client for CrossInk's USB serial file protocol (src/network/UsbSerialFileTransfer.cpp).

Frames: b"CMND" + command byte + u16le path length + path.
  A  list:  text lines "DIR:<path>", "d|<name>", "f|<name>|<size>|0", then "END"
  T  read:  "READY\n", u32le size, raw bytes, u32le CRC-32
  W  write: host sends u32le size after the path; device "READY\n"; host sends 256-byte
            chunks, device ACKs (0x06) after each non-final chunk and once more after the
            last; host sends u32le CRC-32; device "OK\n". Uploads go through a temp file,
            and the device clears that book's cache (keeping progress/stats) on success.
File commands only run while the device is on Home. Log lines share the port, so text
replies are matched by prefix and binary payloads are CRC-checked.
"""
import struct
import time
import zlib

import serial

CHUNK = 256
ACK = 0x06


class CrossInkUsb:
    def __init__(self, port, timeout=0.2):
        self.s = serial.Serial(port, 115200, timeout=timeout)
        time.sleep(0.3)
        self.buf = b""

    def close(self):
        self.s.close()

    # --- low level -------------------------------------------------------
    def _fill(self, deadline):
        # Read whatever has arrived (at least 1 byte, or time out). Asking for a large
        # fixed count would sit out the full timeout on every 1-byte ACK.
        data = self.s.read(max(1, self.s.in_waiting))
        if data:
            self.buf += data
        elif time.time() > deadline:
            raise TimeoutError("serial timeout")

    def _line(self, prefixes, timeout=10):
        """Return the first text line starting with one of `prefixes` (skips log lines)."""
        deadline = time.time() + timeout
        while True:
            while b"\n" in self.buf:
                line, self.buf = self.buf.split(b"\n", 1)
                text = line.rstrip(b"\r").decode("utf-8", "replace")
                if text.startswith("ERR:"):
                    raise IOError(text)
                if any(text.startswith(p) for p in prefixes):
                    return text
            self._fill(deadline)

    def _exact(self, n, timeout=60):
        deadline = time.time() + timeout
        while len(self.buf) < n:
            self._fill(deadline)
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def _cmd(self, letter, path):
        p = path.encode("utf-8")
        self.buf = b""
        self.s.reset_input_buffer()
        self.s.write(b"CMND" + letter.encode() + struct.pack("<H", len(p)) + p)
        self.s.flush()

    # --- commands --------------------------------------------------------
    def home(self, theme):
        """Go Home via the developer theme command (in-memory theme, not saved)."""
        self.buf = b""
        self.s.write(f"CMD:THEME:{theme}\n".encode())
        self.s.flush()
        self._line(["THEME_OK:"])
        time.sleep(3)

    def list(self, path):
        self._cmd("A", path)
        self._line(["DIR:"])
        entries = []
        while True:
            t = self._line(["d|", "f|", "END"])
            if t == "END":
                return entries
            parts = t.split("|")
            if parts[0] == "d":
                entries.append(("d", parts[1], 0))
            else:
                entries.append(("f", parts[1], int(parts[2])))

    def walk(self, path="/"):
        for kind, name, size in self.list(path):
            full = (path.rstrip("/") + "/" + name)
            if kind == "d":
                yield from self.walk(full)
            else:
                yield full, size

    def read(self, path, retries=3):
        for attempt in range(retries):
            self._cmd("T", path)
            try:
                self._line(["READY"])
                size = struct.unpack("<I", self._exact(4))[0]
                data = self._exact(size, timeout=120)
                crc = struct.unpack("<I", self._exact(4))[0]
            except TimeoutError:
                continue
            if zlib.crc32(data) & 0xFFFFFFFF == crc:
                return data
        raise IOError(f"read failed after {retries} attempts: {path}")

    def write(self, path, data):
        self._cmd("W", path)
        self.s.write(struct.pack("<I", len(data)))
        self.s.flush()
        self._line(["READY"])
        for off in range(0, len(data), CHUNK):
            self.s.write(data[off:off + CHUNK])
            self.s.flush()
            self._wait_ack()
        self.s.write(struct.pack("<I", zlib.crc32(data) & 0xFFFFFFFF))
        self.s.flush()
        self._line(["OK"], timeout=60)

    def _wait_ack(self, timeout=45):
        deadline = time.time() + timeout
        while True:
            i = self.buf.find(bytes([ACK]))
            if i >= 0:
                self.buf = self.buf[i + 1:]
                return
            if b"ERR:" in self.buf:
                self._line([])  # raises with the error text
            self._fill(deadline)
