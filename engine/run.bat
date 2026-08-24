@echo off
cls

:: Navigate to script directory
cd /d "%~dp0"

:choose_build
echo Select Build Type:
echo [0] Debug
echo [1] Release
echo.
set /p build_choice="Enter choice (0 or 1): "

if "%build_choice%"=="0" (
    set BUILD_TYPE=Debug
    set BUILD_DIR=build\debug
) else if "%build_choice%"=="1" (
    set BUILD_TYPE=Release
    set BUILD_DIR=build\release
) else (
    cls
    echo Invalid choice. Please enter 0 or 1.
    echo.
    goto choose_build
)

:: Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

:: Navigate into selected build directory
cd "%BUILD_DIR%"

:: Configure with CMake. The Visual Studio generator selects the configuration at build time.
cmake ..\..
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    pause
    exit /b %ERRORLEVEL%
)

:: Build project
cmake --build . --config %BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b %ERRORLEVEL%
)

:: Clear screen before running executable
cls

:: Execute binary (supports Visual Studio subfolders and standard build outputs)
if exist %BUILD_TYPE%\QuantumSimulationStudio.exe (
    %BUILD_TYPE%\QuantumSimulationStudio.exe
) else if exist QuantumSimulationStudio.exe (
    QuantumSimulationStudio.exe
) else (
    echo [ERROR] Executable not found.
)

pause