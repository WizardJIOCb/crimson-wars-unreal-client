[CmdletBinding()]
param(
    [string]$WebAssetsRoot = "C:\Projects\crimson-wars\public\assets",
    [string]$NativeAssetsRoot = "C:\Projects\crimson-wars-native\Content\CrimsonWars\RawAssets"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $WebAssetsRoot)) {
    throw "Web assets folder was not found: $WebAssetsRoot"
}

New-Item -ItemType Directory -Force -Path $NativeAssetsRoot | Out-Null

robocopy $WebAssetsRoot $NativeAssetsRoot /E /NFL /NDL /NJH /NJS /NP | Out-Null
$RobocopyCode = $LASTEXITCODE
if ($RobocopyCode -gt 7) {
    throw "robocopy failed with exit code $RobocopyCode"
}

$env:CW_NATIVE_RAW_ASSETS = $NativeAssetsRoot
@'
import json
import os
from pathlib import Path
from PIL import Image

root = Path(os.environ["CW_NATIVE_RAW_ASSETS"])
converted = 0
skipped = 0
failed = 0

for src in root.rglob("*.webp"):
    dst = src.with_suffix(".png")
    if dst.exists():
        skipped += 1
        continue
    try:
        with Image.open(src) as image:
            image.save(dst, "PNG")
        converted += 1
    except Exception as exc:
        print(f"FAILED {src}: {exc}")
        failed += 1

print(json.dumps({
    "root": str(root),
    "converted": converted,
    "skipped": skipped,
    "failed": failed,
}, ensure_ascii=False))

raise SystemExit(1 if failed else 0)
'@ | python -

if ($LASTEXITCODE -ne 0) {
    throw "WebP conversion failed"
}

Write-Host "Native raw assets are synced: $NativeAssetsRoot"
