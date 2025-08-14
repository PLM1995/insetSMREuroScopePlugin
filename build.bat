@echo off
setlocal

set RUN_ES=false

REM Parse command-line args
IF "%1"=="--release" (
    set BUILD_TYPE=Release
) ELSE IF "%1"=="--debug" (
    set BUILD_TYPE=Debug
) ELSE (
    echo Must specify release type. Build cancelled.
    echo Usage: build.bat [--debug ^| --release] [--run]
    exit /b 1
)

REM Check if user wants to run EuroScope after building
IF "%2"=="--run" (
    set RUN_ES=true
) ELSE (
    set RUN_ES=false
)

REM Generate build system if needed (multi-config)
cmake -S . -B build

REM Build the project at the chosen configuration
cmake --build build --config %BUILD_TYPE%
IF ERRORLEVEL 1 (
    echo Build failed.
    exit /b 1
)

REM Copy DLL to EuroScope plugin folder
copy "build\%BUILD_TYPE%\myPlugIn.dll" "%APPDATA%\EuroScope\UK\Data\Plugin\PLM1995Plugin.dll"
IF ERRORLEVEL 1 (
    echo Failed to copy plugin DLL.
    exit /b 1
)

REM Optionally launch EuroScope
IF "%RUN_ES%"=="true" (
    echo Launching EuroScope...
    start "" /D "%APPDATA%\EuroScope\" "C:\Program Files (x86)\EuroScope\EuroScope.exe"
) ELSE (
    echo EuroScope not launched. Specify --run to start it automatically.
)