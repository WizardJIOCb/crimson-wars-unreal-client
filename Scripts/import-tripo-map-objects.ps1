param(
    [string]$EngineRoot = "C:\UE_5.7"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$projectPath = Join-Path $root "CrimsonWarsNative.uproject"
$scriptPath = Join-Path $root "Scripts\import_tripo_map_objects_unreal.py"
$editorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

if (-not (Test-Path $projectPath)) {
    throw "Project file not found: $projectPath"
}
if (-not (Test-Path $scriptPath)) {
    throw "Import script not found: $scriptPath"
}
if (-not (Test-Path $editorCmd)) {
    throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}

& $editorCmd $projectPath -run=pythonscript -script=$scriptPath -LiveCoding=0 -unattended -nop4
exit $LASTEXITCODE
