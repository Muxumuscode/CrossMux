---
title: LilyGo T5 S3 (CrossMux)
parent: Development
nav_order: 20
---

# LilyGo T5 S3 (CrossMux)

CrossMux is CrossInk with a target for the **LilyGo T5-4.7 S3 Pro Lite**
(ESP32-S3R8, 540x960 16-gray ED047TC1 panel, GT911 touch, PWM frontlight,
PCF8563 RTC, BQ27220/BQ25896 battery). It is based on CrossInk
`release/v2.0.0` (RC-3) and builds with `pio run -e lilygo`.

## Repositories

| Repo | Branch | What it holds |
|---|---|---|
| [Muxumuscode/CrossMux](https://github.com/Muxumuscode/CrossMux) (fork of uxjulia/CrossInk) | `feat/lilygo-t5s3-v2` | App: T5 build target, tall-screen theme sizing, Dashboard totals + shortcut dock, sleep wiring, dev serial commands |
| [Muxumuscode/freeink-sdk](https://github.com/Muxumuscode/freeink-sdk) (fork of Free-Ink/freeink-sdk) | `t5s3-hw-port` | SDK: T5 hardware fixes ported from the [Aurora fork](https://github.com/jetaudio/crosspoint-aurora) (deep-sleep power, frontlight PWM, GPIO2) |

The `freeink-sdk` submodule points at the SDK fork, so a recursive clone gets
both.

## Set up a Windows machine

1. Install Python 3.11+ and Git.
2. Clone with submodules, into a path **without spaces**:

   ```bash
   git clone --recurse-submodules -b feat/lilygo-t5s3-v2 https://github.com/Muxumuscode/CrossMux C:/dev/CrossMux
   ```

3. Install PlatformIO Core: `pip install --user platformio==6.1.19`.
4. Keep PlatformIO's data **out of your user folder** if your Windows user name
   has a space (ESP-IDF refuses paths with spaces). In PowerShell:

   ```powershell
   $env:PLATFORMIO_CORE_DIR = "C:\pio"; $env:PYTHONUTF8 = "1"
   ```

   Set these for every build, or add them as user environment variables.
5. Build once from PowerShell (not Git Bash: ESP-IDF's installer rejects MSYS):

   ```powershell
   python -m platformio run -e lilygo
   ```

   The first build downloads ~2 GB of toolchains and compiles the custom ESP-IDF
   libraries (15-25 minutes). It will most likely **fail at the link step** with
   `No module named 'SCons.Tool.FortranCommon'`. That is a pioarduino version
   clash, not a code problem (it happens on Linux too). Fix it once:

   ```powershell
   python scripts\lilygo\fix_pioarduino_scons.py C:\pio
   ```

6. From then on build with the platform's own PlatformIO, so both nested build
   passes agree on SCons:

   ```powershell
   C:\pio\penv\Scripts\pio.exe run -e lilygo                              # build
   C:\pio\penv\Scripts\pio.exe run -e lilygo -t upload --upload-port COM7 # flash
   C:\pio\penv\Scripts\pio.exe run -e default                             # X4 regression check
   ```

The T5 appears as "USB Serial Device (COMx)" (Espressif VID 303A). If flashing
cannot open the port, hold BOOT, tap RST, release BOOT, and retry.

## Dev tools (`scripts/lilygo/`)

All talk to the device over USB serial and need `pip install pyserial pillow`.

| Script | Use |
|---|---|
| `screenshot.py COM7 out.png` | Save the current screen (framebuffer) as a PNG. |
| `theme_shots.py COM7 outdir [suffix]` | Switch through all 8 Home themes and save each. Uses `CMD:THEME:<n>`, which changes the theme in memory only. |
| `crossink_usb.py` | Python client for CrossInk's USB file protocol: list, read, write. File commands only run while the device is on Home. Uploading a book clears its cache but keeps progress and stats. |
| `scan_meta.py <dir>` | Flag EPUBs whose title/author metadata will display badly. |
| `fix_pioarduino_scons.py` | The Windows SCons fix above. |

Serial developer commands added in this fork: `CMD:SCREENSHOT` (existing, made
reliable on the S3's USB CDC) and `CMD:THEME:<n>`.

## What is in the fork

- **T5 target** (`[env:lilygo]` in `platformio.ini`), board bring-up via
  `BoardT5S3::begin()` in `src/main.cpp`.
- **Tall-screen theme sizing**: `src/components/themes/TallScreen.h` provides
  `tallScreenValue(standard, tall)`; every Home theme uses it for its cover,
  tile, and row sizes. Other devices keep their original values.
- **Dashboard extras** on screens taller than 800 px: a row of lifetime totals.
  On the T5 (tall + touch), Home also has a bottom dock of shortcut icons
  (`DashboardTheme::drawHomeDock()`); the swipe gestures still work.
- **Sleep**: the panel PMIC park hook, frontlight latched off, and a
  stuck-power-line retry wake (`src/main.cpp`, `lib/hal/HalPowerManager.cpp`).
- **Windows build fix** in `scripts/git_branch.py` (pioarduino's include
  shortening wrapper ignores middleware patterns).

## Sleep screen: Custom + Reading Strip

`Settings > Display > Sleep Screen > Custom + Reading Strip` (tall screens only)
draws the custom sleep image (same folder, pinning, and Fit/Crop setting as
Custom) in the band above a 60 px strip, top-aligned. The strip has two rows
(`DashboardTheme::drawSleepReadingStrip()`): the bold book title with the
chapter right-aligned, then `42% · 3h 10m left · done by Dec 3 · 25m today ·
5 day streak`, dropping trailing items that don't fit. A 480x800 X4 image scales to
exactly 540x900. With no book opened yet the image gets the whole screen. The
strip is drawn into the B/W base only; on the T5's overlay grayscale the gray
passes leave its band blank. Converted images are cached per target size, so
the strip mode and plain Custom keep separate cache files.

## Buttons

The T5's case labels its keys **BOOT**, **IO48**, **PWR**, and **RST**, plus the
capacitive Home key. Only BOOT and IO48 reach the firmware:

| Key | Wiring | In books | Elsewhere |
|---|---|---|---|
| BOOT | GPIO0, the app's power key (`BTN_POWER`) | Tap: Previous Page (default); hold: Sleep/Wake | Tap: Up; hold: Sleep/Wake |
| IO48 | PCA9535 IO12 via the board's button hook, delivered as `BTN_DOWN` | Tap: Next Page | Tap: Down; hold 0.6 s: Select |
| BOOT + IO48 | Both together | Configurable combo, Screenshot by default | Same |
| Home | GT911 capacitive key | Configurable (Back/Home by default) | Back/Home |
| PWR | BQ25896 charger /QON, not the ESP32 | Hardware only: a long hold forces a power reset | Same |
| RST | ESP32 EN line | Hardware reset; kept as is | Same |

Implementation: `deviceUsesTwoKeyNavigation()` (`include/DeviceCapabilities.h`)
gates it; `MappedInputManager::updateTwoKeyNavigation()` turns BOOT/IO48 into
one-loop Up/Down/Confirm events outside the reader (taps fire on release, so
holds can mean something else); `src/main.cpp` uses IO48 as the configurable
chord's partner and skips the fixed Power + Down screenshot chord;
`src/ButtonNames.h` supplies the BOOT/IO48 labels. On Dashboard Home the keys
move a highlight across the dock. Reader menus still use the reader mapping.

## Open items

- Text-quality check of upstream's T5 waveform on the glass. Aurora's own
  vendor-waveform port was skipped in favour of upstream's newer driver.
- Aurora's "contact after a polling gap is never a tap" touch fix needs
  re-deriving against upstream's rewritten touch polling.
- List-style Home themes (Classic, Lyra, Lyra 3-Covers, RoundedRaff) still leave
  space below a 5-item menu.
- OTA update checks still read uxjulia/CrossInk releases, which have no T5
  firmware; point them at this fork's releases or disable them for `lilygo`.
- Move to the final `v2.0.0` tag when CrossInk publishes it.
- **Backlight switching by itself** (seen twice; it switched and stayed).
  Suspect: an accidental Home double-tap, whose default action is Toggle
  Frontlight (`homeButtonDoubleTapAction`). The serial log prints
  `Frontlight toggled on/off by Home key` when that path fires. If confirmed,
  consider Aurora's "contact after a polling gap is never a tap" fix for the
  GT911 Home key, or a different T5 default for Home double-tap.

## Parked

- **Game Boy emulator as a dual-boot app.** Try
  [T5S3-GameBoy](https://github.com/ShallowGreen123/T5S3-GameBoy) (CrankBoy core,
  touch controls, ROMs and saves on SD, no hardware mod) standalone first. If its
  speed and ghosting are acceptable and its license allows redistribution: turn
  the unused `spiffs` region of `partitions.csv` (0xC90000, 3.4 MB) into a third
  app slot, add a CrossMux menu entry that sets it as the boot partition and
  restarts, and add a "Back to CrossMux" entry in the emulator. Needs one USB
  reflash for the new partition table; OTA must never target the third slot.
  Not built into CrossMux itself: the T5 app slot is 87.6% full and SD plugins
  cannot run native code.
- **CrossNext features.** [CrossNext](https://github.com/manuelx900/CrossNext)
  (MIT, CrossPoint 2.0.0 + CrossPlay, X4 Pro) is a source of ideas, not code to
  merge. Candidates: Library Collections/Filter (port into CrossMux); its games
  could live in the same third slot as the emulator instead of in the reader.
