@echo off
setlocal EnableExtensions

set "PROJECT_DIR=%~dp0"
set "UPROJECT=%PROJECT_DIR%CrimsonWarsNative.uproject"
set "SCRIPT=%PROJECT_DIR%Scripts\create_native_ground_material_unreal.py"
if not defined UE_ROOT set "UE_ROOT=C:\UE_5.7"
set "UE_CMD=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "NO_PAUSE=0"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--no-pause" set "NO_PAUSE=1"
shift
goto parse_args

:args_done
echo.
echo === Crimson Wars Native: refresh ground material ===
echo Project: "%UPROJECT%"
echo Script:  "%SCRIPT%"
echo.

if not exist "%UPROJECT%" (
    echo ERROR: Project file was not found:
    echo "%UPROJECT%"
    goto failed
)

if not exist "%SCRIPT%" (
    echo ERROR: Python script was not found:
    echo "%SCRIPT%"
    goto failed
)

if not exist "%UE_CMD%" (
    echo ERROR: UnrealEditor-Cmd.exe was not found:
    echo "%UE_CMD%"
    goto failed
)

tasklist /FI "IMAGENAME eq UnrealEditor.exe" 2>NUL | find /I "UnrealEditor.exe" >NUL
if not errorlevel 1 (
    echo ERROR: UnrealEditor.exe is running.
    echo Close Unreal Editor first, then run this file again.
    goto failed
)

call "%UE_CMD%" "%UPROJECT%" -run=pythonscript -script="%SCRIPT%" -unattended -nop4
set "SCRIPT_EXIT=%ERRORLEVEL%"

if not "%SCRIPT_EXIT%"=="0" (
    echo.
    echo MATERIAL REFRESH FAILED with exit code %SCRIPT_EXIT%.
    if "%NO_PAUSE%"=="0" pause
    exit /b %SCRIPT_EXIT%
)

echo.
echo GROUND MATERIAL REFRESHED.
if "%NO_PAUSE%"=="0" pause
exit /b 0

:failed
if "%NO_PAUSE%"=="0" pause
exit /b 1
