@echo off
REM Build script for HelloContact (Windows/MinGW)
REM Usage: scripts\build.bat [target]
REM
REM Environment variables (optional):
REM   CMAKE_PREFIX_PATH  — Path to Qt6 installation (default: C:/msys64/mingw64)
REM   BUILD_DIR          — Build output directory (default: build)

setlocal enabledelayedexpansion

set "BUILD_DIR=%BUILD_DIR:build%"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build"
set "CMAKE_GENERATOR=MinGW Makefiles"
if "%CMAKE_PREFIX_PATH%"=="" set "CMAKE_PREFIX_PATH=C:/msys64/mingw64"

if "%1"=="clean" (
    if exist "%BUILD_DIR%" (
        echo [CLEAN] Removing build directory...
        rmdir /s /q "%BUILD_DIR%"
    )
    echo [CLEAN] Done.
    exit /b 0
)

echo ========================================
echo   HelloContact Build Script (Windows)
echo ========================================
echo.

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo [1/2] Configuring...
cmake -B "%BUILD_DIR%" -G "%CMAKE_GENERATOR%" ^
    -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DHELLO_CONTACT_BUILD_DESKTOP=ON ^
    .
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo [2/2] Building...
cmake --build "%BUILD_DIR%" --config Release
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

echo.
echo [DONE] Build successful!
echo   Terminal: build\apps\terminal\hello_contact_terminal.exe
echo   Desktop:  build\apps\desktop\hello_contact_desktop.exe
