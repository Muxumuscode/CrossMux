# Flash the built CrossMux app to the T5 and make sure it boots.
#
# usage (PowerShell, from anywhere):  scripts\lilygo\flash_t5.ps1 [-Port COM7] [-WaitMinutes 30]
#
# - Waits for the T5's USB serial port (it vanishes while the reader sleeps; wake it with BOOT).
# - Writes .pio\build\lilygo\firmware.bin to app0 (0x10000), with retries.
# - Erases otadata (0xE000): after an OTA update the bootloader runs app1, so a USB
#   flash to app0 would otherwise be ignored.
# - Pulses a normal reset: esptool's own reset can leave the chip in download mode.
# Build first with C:\pio\penv\Scripts\pio.exe run -e lilygo (see docs/development/lilygo-t5s3.md).
param([string]$Port = "", [int]$WaitMinutes = 30)
$ErrorActionPreference = "Continue"
$env:PYTHONUTF8 = "1"
$py = "C:\pio\penv\Scripts\python.exe"
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$firmware = Join-Path $repo ".pio\build\lilygo\firmware.bin"
if (-not (Test-Path $firmware)) { Write-Error "No $firmware; build first."; exit 1 }

function Find-Port {
  if ($Port) { if ([System.IO.Ports.SerialPort]::GetPortNames() -contains $Port) { return $Port } else { return $null } }
  [System.IO.Ports.SerialPort]::GetPortNames() | Where-Object { $_ -ne 'COM1' } | Select-Object -First 1
}

$deadline = (Get-Date).AddMinutes($WaitMinutes); $ok = $false
while (-not $ok -and (Get-Date) -lt $deadline) {
  $p = Find-Port
  if (-not $p) { Start-Sleep -Milliseconds 500; continue }
  Start-Sleep -Seconds 4   # let the board finish booting before esptool grabs the port
  for ($i = 1; $i -le 6 -and -not $ok; $i++) {
    $p = Find-Port; if (-not $p) { break }
    & $py -m esptool --chip esp32s3 --port $p --baud 921600 write-flash 0x10000 $firmware
    $ok = ($LASTEXITCODE -eq 0)
    if (-not $ok) { Start-Sleep -Seconds 2 }
  }
}
if (-not $ok) { Write-Error "Flash failed or no T5 found within $WaitMinutes minutes."; exit 1 }

& $py -m esptool --chip esp32s3 --port $p erase-region 0xe000 0x2000
Start-Sleep -Seconds 3
& $py -I (Join-Path $PSScriptRoot "serial_reset.py") (Find-Port)
Write-Host "Flashed $firmware to $p and rebooted into it."
