param(
    [string]$EngineRoot = "C:\UE_5.7",
    [string]$ProjectPath = "C:\Projects\crimson-wars-native\CrimsonWarsNative.uproject",
    [string]$Target = "CrimsonWarsNativeEditor",
    [string]$Platform = "Win64",
    [string]$Configuration = "Development"
)

$ErrorActionPreference = "Stop"

$ubt = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$buildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"

& "$PSScriptRoot\Check-UnrealPrereqs.ps1" -EngineRoot $EngineRoot -ProjectPath $ProjectPath

Write-Host ""
Write-Host "Generating Unreal project files..." -ForegroundColor Cyan
& $ubt -projectfiles "-project=$ProjectPath" -game -progress

Write-Host ""
Write-Host "Building $Target $Platform $Configuration..." -ForegroundColor Cyan
& $buildBat $Target $Platform $Configuration "-Project=$ProjectPath" -WaitMutex -NoHotReloadFromIDE
