@echo off
setlocal EnableExtensions
cls

set "SCRIPT_DIR=%~dp0"
if "%SCRIPT_DIR:~-1%"=="\" set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
cd /d "%SCRIPT_DIR%"

:: The compiler is intentionally not downloaded. Initialize the installed MSVC toolchain.
call :find_msvc
if errorlevel 1 (
    pause
    exit /b 1
)

call :find_cmake
if errorlevel 1 (
    pause
    exit /b 1
)

if /I "%~1"=="debug" set "build_choice=0"
if /I "%~1"=="release" set "build_choice=1"

:choose_build
if not defined build_choice (
    echo Select Build Type:
    echo [0] Debug
    echo [1] Release
    echo.
    set /p build_choice="Enter choice (0 or 1): "
)

if "%build_choice%"=="0" (
    set BUILD_TYPE=Debug
    set BUILD_DIR=build\v035-debug
    set INSTALL_DIR=%SCRIPT_DIR%\build\qcs-install\debug
) else if "%build_choice%"=="1" (
    set BUILD_TYPE=Release
    set BUILD_DIR=build\v035-release
    set INSTALL_DIR=%SCRIPT_DIR%\build\qcs-install\release
) else (
    cls
    echo Invalid choice. Please enter 0 or 1.
    echo.
    set "build_choice="
    goto choose_build
)

set "BUILD_PATH=%SCRIPT_DIR%\%BUILD_DIR%"

:: Remove a cache created with a different generator (for example, an old VS instance).
if exist "%BUILD_PATH%\CMakeCache.txt" (
    findstr /C:"CMAKE_GENERATOR:INTERNAL=NMake Makefiles" "%BUILD_PATH%\CMakeCache.txt" >nul
    if errorlevel 1 (
        echo Existing CMake cache uses a different generator. Recreating build directory...
        rmdir /s /q "%BUILD_PATH%"
    )
)

:: Create build directory if it doesn't exist
if not exist "%BUILD_PATH%" mkdir "%BUILD_PATH%"

:: Configure with the compiler environment initialized above. NMake is independent of VS version.
"%CMAKE_CMD%" -S "%SCRIPT_DIR%" -B "%BUILD_PATH%" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_INSTALL_PREFIX="%INSTALL_DIR%"
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    pause
    exit /b %ERRORLEVEL%
)

:: Build project
"%CMAKE_CMD%" --build "%BUILD_PATH%"
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b %ERRORLEVEL%
)

:: Export this configuration as a relocatable QCS package.
"%CMAKE_CMD%" --install "%BUILD_PATH%"
if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Library installation failed!
    pause
    exit /b %ERRORLEVEL%
)

:: Clear screen before running executable
cls

:: Execute binary (supports Visual Studio subfolders and standard build outputs)
if exist "%BUILD_PATH%\%BUILD_TYPE%\QuantumSimulationStudio.exe" (
    "%BUILD_PATH%\%BUILD_TYPE%\QuantumSimulationStudio.exe"
) else if exist "%BUILD_PATH%\QuantumSimulationStudio.exe" (
    "%BUILD_PATH%\QuantumSimulationStudio.exe"
) else (
    echo [ERROR] Executable not found.
)

pause
exit /b 0

:find_msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] Visual Studio Installer was not found.
    goto msvc_missing
)

set "VS_INSTALL_PATH="
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do if not defined VS_INSTALL_PATH set "VS_INSTALL_PATH=%%I"
if not defined VS_INSTALL_PATH goto msvc_missing

set "VS_DEV_CMD=%VS_INSTALL_PATH%\Common7\Tools\VsDevCmd.bat"
if not exist "%VS_DEV_CMD%" (
    echo [ERROR] Visual Studio was found, but its developer command script is missing.
    goto msvc_missing
)

call "%VS_DEV_CMD%" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 (
    echo [ERROR] Could not initialize the MSVC developer environment.
    exit /b 1
)
where cl >nul 2>&1
if errorlevel 1 goto msvc_missing

:: Ignore generator settings inherited from another VS or CMake environment.
set "CMAKE_GENERATOR="
set "CMAKE_GENERATOR_INSTANCE="
set "CMAKE_GENERATOR_PLATFORM="
set "CMAKE_GENERATOR_TOOLSET="
echo Using MSVC: %VS_INSTALL_PATH%
exit /b 0

:msvc_missing
echo [ERROR] A compatible MSVC C++ toolchain was not found.
echo Install Visual Studio or Build Tools with the Desktop development with C++ workload.
exit /b 1

:find_cmake
where.exe cmake >nul 2>&1
if errorlevel 1 goto download_cmake
for /f "delims=" %%I in ('where.exe cmake') do if not defined CMAKE_CMD set "CMAKE_CMD=%%I"
echo Using CMake: %CMAKE_CMD%
exit /b 0

:download_cmake
set "CMAKE_VERSION=3.31.7"
set "CMAKE_ROOT=%SCRIPT_DIR%\enginetools\cmake-%CMAKE_VERSION%-windows-x86_64"
set "CMAKE_CMD=%CMAKE_ROOT%\bin\cmake.exe"
if not exist "%CMAKE_CMD%" (
    set "CMAKE_ROOT=%SCRIPT_DIR%\tools\cmake-%CMAKE_VERSION%-windows-x86_64"
    set "CMAKE_CMD=%CMAKE_ROOT%\bin\cmake.exe"
)
if exist "%CMAKE_CMD%" (
    echo Using local CMake: %CMAKE_CMD%
    exit /b 0
)

echo CMake was not found. Downloading portable CMake %CMAKE_VERSION% locally...
set "CMAKE_ZIP=%TEMP%\qcs-cmake-%CMAKE_VERSION%.zip"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; New-Item -ItemType Directory -Force -Path '%SCRIPT_DIR%tools' | Out-Null; Invoke-WebRequest -Uri 'https://github.com/Kitware/CMake/releases/download/v%CMAKE_VERSION%/cmake-%CMAKE_VERSION%-windows-x86_64.zip' -OutFile '%CMAKE_ZIP%'; Expand-Archive -Path '%CMAKE_ZIP%' -DestinationPath '%SCRIPT_DIR%tools' -Force; Remove-Item '%CMAKE_ZIP%' -Force"
if errorlevel 1 (
    echo [ERROR] Could not download CMake. Check internet access or install CMake manually.
    exit /b 1
)
if not exist "%CMAKE_CMD%" (
    echo [ERROR] The downloaded CMake installation is incomplete.
    exit /b 1
)
echo Using local CMake: %CMAKE_CMD%
exit /b 0