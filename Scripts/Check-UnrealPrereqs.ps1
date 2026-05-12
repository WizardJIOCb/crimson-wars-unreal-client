param(
    [string]$EngineRoot = "C:\UE_5.7",
    [string]$ProjectPath = "C:\Projects\crimson-wars-native\CrimsonWarsNative.uproject"
)

$ErrorActionPreference = "Stop"

function Test-File($Path, $Label) {
    $exists = Test-Path -LiteralPath $Path
    [pscustomobject]@{
        Check = $Label
        Path = $Path
        OK = $exists
    }
}

$ubt = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$buildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"

$checks = @(
    Test-File $EngineRoot "Engine root"
    Test-File $editor "UnrealEditor"
    Test-File $ubt "UnrealBuildTool"
    Test-File $buildBat "Build.bat"
    Test-File $ProjectPath "Project"
    Test-File $vswhere "vswhere"
)

$vs2022 = @()
if (Test-Path -LiteralPath $vswhere) {
    $vs2022 = & $vswhere -all -products * -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}

$msvc143 = Get-ChildItem -LiteralPath "C:\Program Files\Microsoft Visual Studio","C:\Program Files (x86)\Microsoft Visual Studio" -Recurse -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "VC\\Tools\\MSVC\\14\.(3[8-9]|4[0-9])" } |
    Select-Object -First 1 -ExpandProperty FullName

$netFxSdk = ""
$netFxSdkKeys = @(
    "HKLM:\SOFTWARE\Microsoft\Microsoft SDKs\NETFXSDK",
    "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Microsoft SDKs\NETFXSDK"
)
foreach ($key in $netFxSdkKeys) {
    if (-not (Test-Path $key)) {
        continue
    }
    $netFxSdk = Get-ChildItem $key -ErrorAction SilentlyContinue |
        ForEach-Object { Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue } |
        Where-Object { $_.InstallationFolder } |
        Select-Object -First 1 -ExpandProperty InstallationFolder
    if ($netFxSdk) {
        break
    }
}

$checks += [pscustomobject]@{
    Check = "Visual Studio 2022 C++"
    Path = ($vs2022 -join "; ")
    OK = [bool]$vs2022
}

$checks += [pscustomobject]@{
    Check = "MSVC v143 14.38+"
    Path = [string]$msvc143
    OK = [bool]$msvc143
}

$checks += [pscustomobject]@{
    Check = ".NET Framework SDK"
    Path = [string]$netFxSdk
    OK = [bool]$netFxSdk
}

$checks | Format-Table -AutoSize

if ($checks.OK -contains $false) {
    Write-Host ""
    Write-Host "Missing requirements. Unreal 5.7 needs Visual Studio 2022 17.8+ with MSVC v143 14.38+ and .NET Framework Developer Pack." -ForegroundColor Yellow
    Write-Host "Winget packages: Microsoft.VisualStudio.2022.BuildTools, Microsoft.DotNet.Framework.DeveloperPack_4" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "Unreal native prerequisites look ready." -ForegroundColor Green
