# Apply CrossMux's pending freeink-sdk patches (scripts/lilygo/sdk-patches/*.patch)
# to the freeink-sdk submodule, as commits on its t5s3-hw-port branch.
#
# These are SDK changes made in a session that could not push to
# Muxumuscode/freeink-sdk. After applying: build and flash, then push the SDK
# branch and commit the advanced submodule gitlink in CrossMux, e.g.
#   git -C freeink-sdk push <fork remote> t5s3-hw-port
#   git add freeink-sdk; git commit -m "chore: advance freeink-sdk"
#
# Usage (from the CrossMux folder):  powershell -File scripts\lilygo\apply_sdk_patches.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sdk = Join-Path $root "freeink-sdk"
$patches = Get-ChildItem (Join-Path $PSScriptRoot "sdk-patches") -Filter *.patch | Sort-Object Name
if (-not $patches) { Write-Host "No SDK patches to apply."; exit 0 }

# The fork's remote: whichever remote points at Muxumuscode/freeink-sdk (a fresh submodule
# clone calls it origin; an older clone may have it under another name, such as mux).
$fork = git -C $sdk remote -v | Select-String 'Muxumuscode/freeink-sdk' | ForEach-Object { ($_.Line -split '\s+')[0] } | Select-Object -First 1
if (-not $fork) { Write-Error "No freeink-sdk remote points at Muxumuscode/freeink-sdk."; exit 1 }
git -C $sdk fetch $fork t5s3-hw-port
git -C $sdk checkout -B t5s3-hw-port "$fork/t5s3-hw-port"
foreach ($p in $patches) {
  # Skip a patch whose change is already on the branch (subject match).
  $subject = (Select-String -Path $p.FullName -Pattern '^Subject: \[PATCH\] (.*)$').Matches[0].Groups[1].Value
  if (git -C $sdk log --format=%s "$fork/t5s3-hw-port" | Select-String -SimpleMatch $subject) {
    Write-Host "Already applied: $($p.Name)"
    continue
  }
  git -C $sdk am --3way $p.FullName
  Write-Host "Applied: $($p.Name)"
}
git -C $sdk log --oneline -3
