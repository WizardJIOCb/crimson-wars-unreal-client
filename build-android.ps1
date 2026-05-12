param(
    [string]$EngineRoot = "C:\UE_5.7",
    [string]$AndroidSdk = "$env:LOCALAPPDATA\Android\Sdk",
    [string]$NdkVersion = "27.1.12297006",
    [string]$JavaHome = "C:\Program Files\Android\Android Studio\jbr",
    [string]$ArchiveDir = "",
    [string]$Configuration = "Development",
    [switch]$NoPause
)

$ErrorActionPreference = "Stop"

function Pause-IfNeeded {
    if (-not $NoPause) {
        Write-Host ""
        Read-Host "Press Enter to close"
    }
}

try {
    $root = Split-Path -Parent $MyInvocation.MyCommand.Path
    $projectPath = Join-Path $root "CrimsonWarsNative.uproject"
    $uatPath = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
    $ndkRoot = Join-Path $AndroidSdk "ndk\$NdkVersion"

    if ([string]::IsNullOrWhiteSpace($ArchiveDir)) {
        $ArchiveDir = Join-Path $root "Saved\AndroidBuild"
    }

    foreach ($path in @($projectPath, $uatPath, $AndroidSdk, $ndkRoot, $JavaHome)) {
        if (-not (Test-Path $path)) {
            throw "Required path not found: $path"
        }
    }

    $env:ANDROID_HOME = $AndroidSdk
    $env:ANDROID_SDK_ROOT = $AndroidSdk
    $env:NDKROOT = $ndkRoot
    $env:NDK_ROOT = $ndkRoot
    $env:JAVA_HOME = $JavaHome

    Write-Host "Building Android APK to $ArchiveDir ..." -ForegroundColor Cyan
    & $uatPath BuildCookRun `
        -project="$projectPath" `
        -noP4 `
        -platform=Android `
        -clientconfig="$Configuration" `
        -cook `
        -map=/Game/Maps/L_NativeMenuSandbox `
        -build `
        -stage `
        -pak `
        -package `
        -archive `
        -archivedirectory="$ArchiveDir" `
        -utf8output

    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    Write-Host ""
    Write-Host "Android APK build completed: $ArchiveDir" -ForegroundColor Green
    Pause-IfNeeded
} catch {
    Write-Host ""
    Write-Host "Android build failed: $($_.Exception.Message)" -ForegroundColor Red
    Pause-IfNeeded
    exit 1
}
