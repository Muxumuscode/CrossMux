"""Work around pioarduino's SCons version clash on Windows.

pioarduino 55.03.311's platform.json pins tool-scons 4.8.1, while the pioarduino
PlatformIO Core it installs into its own venv (penv) wants SCons 4.11.1. Builds that
use custom_sdkconfig run PlatformIO a second time from inside the first, each run
"repairs" tool-scons to the version it expects, and the outer run loses its SCons
files mid-build. The link step then fails with:

    ModuleNotFoundError: No module named 'SCons.Tool.FortranCommon'

Pointing the platform at the same SCons as the core stops the swap. Run this once
after the first (failed) build has downloaded the platform; re-run it if the
platform is ever re-installed.

usage: python fix_pioarduino_scons.py [PLATFORMIO_CORE_DIR]   (default: C:/pio)
"""
import json
import pathlib
import shutil
import sys

SCONS_URL = "https://github.com/pioarduino/scons/releases/download/4.11.1/scons-local-4.11.1.tar.gz"
SCONS_PACKAGE_VERSION = "4.41101.0"

core = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "C:/pio")
platform_json = core / "platforms" / "espressif32" / "platform.json"
if not platform_json.exists():
    sys.exit(f"{platform_json} not found: run one build first so PlatformIO downloads the platform")

data = json.loads(platform_json.read_text(encoding="utf-8"))
scons = data["packages"]["tool-scons"]
if scons.get("package-version") == SCONS_PACKAGE_VERSION:
    print("already patched")
    sys.exit(0)

backup = platform_json.with_suffix(".json.orig")
if not backup.exists():
    shutil.copy2(platform_json, backup)
scons["package-version"] = SCONS_PACKAGE_VERSION
scons["version"] = SCONS_URL
platform_json.write_text(json.dumps(data, indent=2), encoding="utf-8")
print(f"patched {platform_json} (backup: {backup.name})")
