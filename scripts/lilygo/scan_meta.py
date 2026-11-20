"""Report EPUB title/author metadata (run on a copy of the SD card, e.g. pulled with crossink_usb.py) and flag books CrossInk will display badly.

usage: scan_meta.py <books_dir>
"""
import os
import re
import sys
import zipfile
import xml.etree.ElementTree as ET

DC = "{http://purl.org/dc/elements/1.1/}"
CONTAINER_NS = "{urn:oasis:names:tc:opendocument:xmlns:container}"


def opf_path(z):
    root = ET.fromstring(z.read("META-INF/container.xml"))
    return root.find(f".//{CONTAINER_NS}rootfile").get("full-path")


def read_meta(path):
    with zipfile.ZipFile(path) as z:
        opf = opf_path(z)
        root = ET.fromstring(z.read(opf))
    titles = [(e.text or "").strip() for e in root.iter(f"{DC}title")]
    creators = [(e.text or "").strip() for e in root.iter(f"{DC}creator")]
    return opf, titles, creators


def problems(title, creator):
    out = []
    t = title.strip()
    if not t:
        out.append("no title")
    elif re.fullmatch(r"[\d\s\-_.xX]+", t):
        out.append("numeric/ISBN title")
    elif not re.search(r"[A-Za-z]{2,}", t):
        out.append("title has no words")
    elif re.fullmatch(r"(?i)(unknown|untitled|title|book|ebook|document)\b.*", t):
        out.append("placeholder title")
    c = creator.strip()
    if not c or re.fullmatch(r"(?i)unknown( author)?|anonymous|n/?a|-", c):
        out.append("no/unknown author")
    return out


def main():
    root_dir = sys.argv[1]
    flagged = []
    for dirpath, _, names in os.walk(root_dir):
        for n in sorted(names):
            if not n.lower().endswith(".epub"):
                continue
            p = os.path.join(dirpath, n)
            rel = "/" + os.path.relpath(p, root_dir).replace("\\", "/")
            try:
                _, titles, creators = read_meta(p)
            except Exception as e:  # noqa: BLE001 - report and continue
                flagged.append((rel, "", "", [f"unreadable: {e}"]))
                continue
            title = titles[0] if titles else ""
            creator = creators[0] if creators else ""
            probs = problems(title, creator)
            if probs:
                flagged.append((rel, title, creator, probs))
    for rel, title, creator, probs in flagged:
        print(f"{rel}\n    title={title!r} author={creator!r}  -> {', '.join(probs)}")
    print(f"\n{len(flagged)} flagged")


if __name__ == "__main__":
    main()
