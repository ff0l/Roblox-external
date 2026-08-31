@echo off
setlocal EnableExtensions
cd /d "%~dp0"

if /i "%~1"=="--help" goto :help
if /i "%~1"=="-h" goto :help
if /i "%~1"=="/?" goto :help

set "PRESET=windows-release"
if /i "%~1"=="--debug" set "PRESET=windows-debug"

set "VSDEV="
if defined VSINSTALLDIR if exist "%VSINSTALLDIR%\Common7\Tools\VsDevCmd.bat" set "VSDEV=%VSINSTALLDIR%\Common7\Tools\VsDevCmd.bat"

if not defined VSDEV if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "%ProgramFiles%\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat" set "VSDEV=%ProgramFiles%\Microsoft Visual Studio\18\Professional\Common7\Tools\VsDevCmd.bat"
if not defined VSDEV if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VSDEV=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"

if not defined VSDEV (
    echo Error: Visual Studio with C++ and CMake was not found.
    echo Install Visual Studio 2022 or newer with "Desktop development with C++".
    echo Example: build.bat
    exit /b 1
)

if not exist "third_party\custom-framework\CMakeLists.txt" (
    echo Error: third_party\custom-framework is missing.
    echo Download the full source zip from GitHub, not a sparse checkout.
    exit /b 1
)

if not exist "third_party\fonts\Poppins-Regular.ttf" (
    echo Error: third_party\fonts is missing.
    echo Download the full source zip from GitHub.
    exit /b 1
)

echo Using: %VSDEV%
call "%VSDEV%" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 (
    echo Error: VsDevCmd failed.
    exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: cmake was not on PATH after VsDevCmd.
    echo In the Visual Studio installer, enable "C++ CMake tools for Windows".
    exit /b 1
)

cmake --preset %PRESET%
if errorlevel 1 (
    echo Error: cmake configure failed.
    echo Example: build.bat
    exit /b 1
)

cmake --build --preset %PRESET%
if errorlevel 1 (
    echo Error: build failed.
    echo Example: build.bat
    exit /b 1
)

set "OUT=build\%PRESET%\ff0l.exe"
if not exist "%OUT%" (
    echo Error: %OUT% was not built.
    exit /b 1
)

echo Built %OUT%
echo Run: "%~dp0%OUT%"
exit /b 0

:help
echo Build FF0L. Uses the vendored framework and fonts in third_party.
echo No extra clones. Needs Visual Studio 2022 or newer with C++ and CMake.
echo.
echo Examples:
echo   build.bat
echo   build.bat --debug
echo.
echo Output: build\windows-release\ff0l.exe
exit /b 0
