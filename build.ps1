param(
    [string]$EngineRoot = "C:\UE_5.7",
    [switch]$CloseEditor,
    [switch]$ForceCloseEditor,
    [switch]$NoPause
)

$ErrorActionPreference = "Stop"

function Pause-IfNeeded {
    if (-not $NoPause) {
        Write-Host ""
        Read-Host "Press Enter to close"
    }
}

function Close-UnrealProcesses {
    param(
        [switch]$Force
    )

    $processes = Get-Process UnrealEditor,LiveCodingConsole -ErrorAction SilentlyContinue
    if (-not $processes) {
        return
    }

    Write-Host "Unreal Editor / Live Coding is running:" -ForegroundColor Yellow
    $processes | Select-Object ProcessName,Id,Path | Format-Table -AutoSize

    if (-not $CloseEditor -and -not $Force) {
        $answer = Read-Host "Close them before building Editor target? [Y/N]"
        if ($answer -notmatch '^(y|yes|д|да)$') {
            throw "Close Unreal Editor and LiveCodingConsole, then run this script again."
        }
    }

    foreach ($process in $processes) {
        Write-Host "Requesting close: $($process.ProcessName) [$($process.Id)]"
        try {
            [void]$process.CloseMainWindow()
        } catch {
            Write-Host "Could not request graceful close for $($process.ProcessName)." -ForegroundColor Yellow
        }
    }

    $deadline = (Get-Date).AddSeconds(20)
    do {
        Start-Sleep -Milliseconds 500
        $remaining = Get-Process UnrealEditor,LiveCodingConsole -ErrorAction SilentlyContinue
    } while ($remaining -and (Get-Date) -lt $deadline)

    $remaining = Get-Process UnrealEditor,LiveCodingConsole -ErrorAction SilentlyContinue
    if ($remaining -and $Force) {
        Write-Host "Force closing remaining Unreal processes..." -ForegroundColor Yellow
        $remaining | Stop-Process -Force
        Start-Sleep -Seconds 1
        $remaining = Get-Process UnrealEditor,LiveCodingConsole -ErrorAction SilentlyContinue
    }

    if ($remaining) {
        throw "Unreal Editor is still running. Save/close it manually or run with -ForceCloseEditor."
    }
}

try {
    $root = Split-Path -Parent $MyInvocation.MyCommand.Path
    $projectPath = Join-Path $root "CrimsonWarsNative.uproject"
    $buildScript = Join-Path $root "Scripts\Build-Unreal.ps1"

    if (-not (Test-Path $projectPath)) {
        throw "Project file not found: $projectPath"
    }
    if (-not (Test-Path $buildScript)) {
        throw "Build script not found: $buildScript"
    }

    Close-UnrealProcesses -Force:$ForceCloseEditor

    Write-Host ""
    Write-Host "Building CrimsonWarsNativeEditor Win64 Development..." -ForegroundColor Cyan
    & powershell -ExecutionPolicy Bypass -File $buildScript `
        -EngineRoot $EngineRoot `
        -ProjectPath $projectPath `
        -Target CrimsonWarsNativeEditor `
        -Platform Win64 `
        -Configuration Development

    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    Write-Host ""
    Write-Host "Editor target build completed. You can open CrimsonWarsNative.uproject now." -ForegroundColor Green
    Pause-IfNeeded
} catch {
    Write-Host ""
    Write-Host "Build failed: $($_.Exception.Message)" -ForegroundColor Red
    Pause-IfNeeded
    exit 1
}
