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
| `flash_t5.ps1 [-Port COM7]` | Flash the built app over USB: waits for the T5, retries, erases otadata so app0 boots even after an OTA update, then resets. |
| `screen_tour.py COM7 outdir` | Open every `CMD:OPEN` screen, screenshot each, and save a contact sheet (`sheet.png`). |
| `serial_reset.py COM7` | Normal reset over USB (esptool's reset can leave the chip in download mode). |

Serial developer commands added in this fork: `CMD:SCREENSHOT` (existing, made
reliable on the S3's USB CDC), `CMD:THEME:<n>`, and `CMD:SLEEPPREVIEW`, which
draws the sleep screen without sleeping (USB is off once the device really
sleeps). Send `CMD:SLEEPPREVIEW` and `CMD:SCREENSHOT` together, or Home redraws
over the preview first; `CMD:THEME:6` returns to Home.

`CMD:OPEN:<screen>` opens a screen by name and replies `OPEN_OK:<screen>` or
`OPEN_ERR:<screen>`. It is an allowlist of views only (no delete, toggle, sync,
rotate, or network actions): `home`, `controlcenter`, `library`, `files`,
`settings`, `plugins`, `reader`, `drawer0`..`drawer4` (the reader menu on the
Font, Layout, More, Location, or Settings tab), and the in-book views
`contents`, `percent`, `bookstats`, `readeroptions`, `controls`, `bookmarks`,
`clippings`, `lookuphistory`, `statusbar`. Reader screens open the last book
read. `screen_tour.py` walks all of them.

Flashing note: after an OTA update the bootloader runs **app1**, so writing
`firmware.bin` to app0 (0x10000) alone changes nothing on the device. Erase
otadata (`esptool erase-region 0xe000 0x2000`) or use `flash_t5.ps1`, which
does it for you.

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
- **CrossMux branding**: the boot logo (`src/images/Logo120.h`, an ink drop in
  an open book, stored rotated like the panel framebuffer), the name under it
  with a "built on CrossInk" line, the default sleep screen name
  (`STR_CROSSINK`), and the web portal's logo, wordmark, titles, and footer.

## Releases and OTA updates

The `lilygo` build checks
`https://api.github.com/repos/Muxumuscode/CrossMux/releases/latest` and installs
the release's `crossmux-t5-app.bin` when its tag is newer than the running
version. GitHub's "latest" never returns pre-releases, so pre-releases are a safe
way to share test builds; devices only update to full releases.

Versions are the CrossInk base plus a CrossMux build number: `2.0.0.1`,
`2.0.0.2`, and so on (`[crossmux] version` in `platformio.ini`, read by
`scripts/git_branch.py`). When CrossMux moves to a newer CrossInk base, restart
at `<base>.1`.

To publish an update:

1. Bump `[crossmux] version` in `platformio.ini` and commit.
2. Build: `pio run -e lilygo`. The log prints `CrossMux build version: 2.0.0.N`.
3. Create a GitHub release tagged `v2.0.0.N` (the tag must match the version).
4. Upload `.pio/build/lilygo/firmware.bin` as `crossmux-t5-app.bin` (the name
   OTA looks for), plus the full image as `crossmux-t5-full.bin` for USB
   installs.
5. Publish it as a full release. Leave "Set as a pre-release" unticked.

Devices running a build from before this change call themselves `dev` and never
see an update as newer; flash a `2.0.0.1` or later build over USB once.

## Sleep screen: Custom + Reading Strip

`Settings > Display > Sleep Screen > Custom + Reading Strip` (tall screens only)
draws the custom sleep image (same folder, pinning, and Fit/Crop setting as
Custom) in the band above a 60 px strip, top-aligned. The strip is one line
(`DashboardTheme::drawSleepReadingStrip()`) in the largest face that fits
(Lexend Deca 16, then 14, then the UI font; its glyphs are prewarmed first): the
bold title on the left, then `3h 6m left`, a flame icon with the streak, and the
bold percentage, right-aligned. On long titles the streak drops first, then
time left; the title is only shortened if it cannot fit beside the percentage.
A 6 px progress bar sits 6 px above the bottom edge, clear of the recessed
panel's bezel shadow, with a 1 px track for the unread part. A 480x800 X4 image scales to
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
- Move to the final `v2.0.0` tag when CrossInk publishes it.
- **Backlight switching by itself** (seen twice; it switched and stayed).
  Suspect: an accidental Home double-tap, whose default action is Toggle
  Frontlight (`homeButtonDoubleTapAction`). The serial log prints
  `Frontlight toggled on/off by Home key` when that path fires. Mitigated in
  `52884edb`: Home taps under 50 ms are ignored as pad flicker, and the T5's
  Home double-tap now defaults to Ignore (existing settings keep their saved
  value: change it under Settings > Controls > Home button gestures). Light
  sleep polls touch only every ~150 ms, so a flicker during it can still pass;
  watch for a recurrence.
  Recurred while navigating Settings by touch (Home pad untouched). The pad
  shares the GT911 with the screen, so a screen contact near the bottom edge or
  a palm on the bezel can set the key bit. Home taps that start during, or
  within 300 ms after, a screen contact are now ignored and logged as
  `Ignored a Home-key tap that started during a screen touch`. If the light
  still switches, capture a serial log: `Frontlight toggled ... by ...` names
  the trigger, and `FrontlightMgr apply: ... lit=0` with no toggle line before
  it means the change came from somewhere else.

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
