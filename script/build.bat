@echo off
setlocal

REM Usage:
REM   build.bat           Build Release only (default)
REM   build.bat -r        Build and run Release
REM   build.bat -d        Build Debug only
REM   build.bat -d -r     Build and run Debug
REM   build.bat 1         Clean build, rebuild, Release only
REM   build.bat -d 1 -r   Clean build, rebuild, and run Debug

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fI"

set "BUILD_TYPE=Release"
set "CONFIG_PRESET=release"
set "BUILD_PRESET=build-release"
set "BUILD_DIR=Release"
set "WINDOW_TITLE=Gg Release"
set "REBUILD=0"
set "RUN_AFTER_BUILD=0"
set "SHOW_HELP=0"
set "EXE_PATH=%REPO_ROOT%\build\bin\Release\Gg.exe"

for %%A in (%*) do (
    if /I "%%~A"=="-d" (
        set "BUILD_TYPE=Debug"
        set "CONFIG_PRESET=debug"
        set "BUILD_PRESET=build-debug"
        set "BUILD_DIR=Debug"
        set "WINDOW_TITLE=Gg Debug"
        set "EXE_PATH=%REPO_ROOT%\build\bin\Debug\Gg.exe"
    ) else if /I "%%~A"=="-h" (
        set "SHOW_HELP=1"
    ) else if /I "%%~A"=="-r" (
        set "RUN_AFTER_BUILD=1"
    ) else if "%%~A"=="0" (
        REM no-op for compatibility
    ) else if "%%~A"=="1" (
        set "REBUILD=1"
    ) else (
        echo Invalid argument: %%~A
        echo Use -d or -D for Debug, -r or -R to run, 1 for clean rebuild.
        exit /b 1
    )
)

if "%SHOW_HELP%"=="1" (
    echo Usage:
    echo   build.bat           Build Release only ^(default^)
    echo   build.bat -r        Build and run Release
    echo   build.bat -d        Build Debug only
    echo   build.bat -d -r     Build and run Debug
    echo   build.bat 1         Clean build, rebuild, Release only
    echo   build.bat -d 1 -r   Clean build, rebuild, and run Debug
    echo   build.bat -h        Show this help
    exit /b 0
)

pushd "%REPO_ROOT%" >nul
if errorlevel 1 (
    echo Failed to enter repo root: %REPO_ROOT%
    exit /b 1
)

if "%REBUILD%"=="1" (
    echo Removing build directory...
    if exist "build" rmdir /s /q "build"
    if exist "build" (
        echo Failed to remove build directory.
        popd >nul
        exit /b 1
    )
)

echo Configuring %BUILD_TYPE% preset...
cmake --preset %CONFIG_PRESET%
if errorlevel 1 (
    echo Configure failed.
    popd >nul
    exit /b 1
)

echo Building %BUILD_TYPE% preset...
cmake --build --preset %BUILD_PRESET%
if errorlevel 1 (
    echo Build failed.
    popd >nul
    exit /b 1
)

if "%RUN_AFTER_BUILD%"=="1" (
    if not exist "%EXE_PATH%" (
        echo Built executable not found: %EXE_PATH%
        popd >nul
        exit /b 1
    )

    echo Running %EXE_PATH%
    start "%WINDOW_TITLE%" "%EXE_PATH%"
)

popd >nul
exit /b 0
