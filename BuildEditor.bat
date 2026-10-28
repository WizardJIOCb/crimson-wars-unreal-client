@echo off
setlocal EnableExtensions

set "PROJECT_DIR=%~dp0"
set "UPROJECT=%PROJECT_DIR%CrimsonWarsNative.uproject"
if not defined UE_ROOT set "UE_ROOT=C:\UE_5.7"
set "UE_BUILD=%UE_ROOT%\Engine\Build\BatchFiles\Build.bat"
set "UE_EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
set "TARGET=CrimsonWarsNativeEditor"
set "PLATFORM=Win64"
set "CONFIG=Development"
set "OPEN_EDITOR=0"
set "NO_PAUSE=0"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--open" set "OPEN_EDITOR=1"
if /I "%~1"=="--no-pause" set "NO_PAUSE=1"
shift
goto parse_args

:args_done
echo.
echo === Crimson Wars Native: build Unreal Editor target ===
echo Target:  %TARGET% %PLATFORM% %CONFIG%
echo Project: "%UPROJECT%"
echo UE:      "%UE_ROOT%"
echo.

if not exist "%UPROJECT%" (
    echo ERROR: Project file was not found:
    echo "%UPROJECT%"
    goto failed
)

if not exist "%UE_BUILD%" (
    echo ERROR: Unreal Build.bat was not found:
    echo "%UE_BUILD%"
    goto failed
)

tasklist /FI "IMAGENAME eq UnrealEditor.exe" 2>NUL | find /I "UnrealEditor.exe" >NUL
if not errorlevel 1 (
    echo ERROR: UnrealEditor.exe is running.
    echo Close Unreal Editor first, then run this file again.
    goto failed
)

tasklist /FI "IMAGENAME eq LiveCodingConsole.exe" 2>NUL | find /I "LiveCodingConsole.exe" >NUL
if not errorlevel 1 (
    echo ERROR: LiveCodingConsole.exe is running.
    echo Close it, or close Unreal Editor fully, then run this file again.
    goto failed
)

call "%UE_BUILD%" %TARGET% %PLATFORM% %CONFIG% "-Project=%UPROJECT%" -WaitMutex -NoHotReloadFromIDE
set "BUILD_EXIT=%ERRORLEVEL%"

if not "%BUILD_EXIT%"=="0" (
    echo.
    echo BUILD FAILED with exit code %BUILD_EXIT%.
    if "%NO_PAUSE%"=="0" pause
    exit /b %BUILD_EXIT%
)

echo.
echo BUILD SUCCEEDED.

if "%OPEN_EDITOR%"=="1" (
    if exist "%UE_EDITOR%" (
        echo Opening Unreal Editor...
        start "" "%UE_EDITOR%" "%UPROJECT%"
    ) else (
        echo WARNING: UnrealEditor.exe was not found:
        echo "%UE_EDITOR%"
    )
)

if "%NO_PAUSE%"=="0" pause
exit /b 0

:failed
if "%NO_PAUSE%"=="0" pause
exit /b 1
