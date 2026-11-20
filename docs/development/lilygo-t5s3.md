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
   clash, not a code problem. Fix it once:

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
