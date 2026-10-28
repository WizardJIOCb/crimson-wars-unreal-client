@echo off
setlocal EnableExtensions

set "PROJECT_DIR=%~dp0"
set "UPROJECT=%PROJECT_DIR%CrimsonWarsNative.uproject"
set "UE_ROOT=C:\UE_5.7"
set "UE_BUILD=%UE_ROOT%\Engine\Build\BatchFiles\Build.bat"
set "UE_EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"

echo.
echo === Crimson Wars Native build ===
echo Project: "%UPROJECT%"
echo.

if not exist "%UPROJECT%" (
    echo ERROR: Project file was not found.
    echo "%UPROJECT%"
    pause
    exit /b 1
)

if not exist "%UE_BUILD%" (
    echo ERROR: Unreal Build.bat was not found.
    echo "%UE_BUILD%"
    pause
    exit /b 1
)

if not exist "%UE_EDITOR%" (
    echo ERROR: UnrealEditor.exe was not found.
    echo "%UE_EDITOR%"
    pause
    exit /b 1
)

echo Closing Unreal Editor and Live Coding if they are running...
call :close_process UnrealEditor.exe
call :close_process LiveCodingConsole.exe

echo.
echo Building CrimsonWarsNativeEditor...
call "%UE_BUILD%" CrimsonWarsNativeEditor Win64 Development "-Project=%UPROJECT%" -WaitMutex -NoHotReloadFromIDE
set "BUILD_EXIT=%ERRORLEVEL%"

if not "%BUILD_EXIT%"=="0" (
    echo.
    echo BUILD FAILED with exit code %BUILD_EXIT%.
    pause
    exit /b %BUILD_EXIT%
)

echo.
echo Build succeeded. Opening project in Unreal Editor...
start "" "%UE_EDITOR%" "%UPROJECT%"
exit /b 0

:close_process
set "PROC=%~1"
tasklist /FI "IMAGENAME eq %PROC%" 2>NUL | find /I "%PROC%" >NUL
if errorlevel 1 (
    echo %PROC% is not running.
    exit /b 0
)

echo Requesting %PROC% to close...
taskkill /IM "%PROC%" >NUL 2>NUL
timeout /T 3 /NOBREAK >NUL

tasklist /FI "IMAGENAME eq %PROC%" 2>NUL | find /I "%PROC%" >NUL
if errorlevel 1 (
    echo %PROC% closed.
    exit /b 0
)

echo Force closing %PROC%...
taskkill /F /IM "%PROC%" >NUL 2>NUL
timeout /T 1 /NOBREAK >NUL
exit /b 0
