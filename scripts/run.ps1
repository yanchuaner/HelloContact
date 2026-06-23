# HelloContact UI Launcher (MSYS2/Git Bash compatible)
$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $scriptDir
Set-Location $root

Write-Host "========================================"
Write-Host "  HelloContact - UI Launcher"
Write-Host "========================================"
Write-Host ""

$buildDir = "build"
$exePath = "$buildDir\apps\desktop\hello_contact_desktop.exe"

# 1. Check if already built
if (Test-Path $exePath) {
    Write-Host "[OK] Found prebuilt exe, launching..." -ForegroundColor Green
    Start-Process $exePath
    exit 0
}

# 2. Check build tools
Write-Host "[INFO] No exe found, will auto-build..." -ForegroundColor Yellow
Write-Host ""

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "[FAIL] cmake not found. Please install CMake and add it to PATH." -ForegroundColor Red
    Pause; exit 1
}
Write-Host "[OK] cmake found" -ForegroundColor Green

$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $gpp) {
    Write-Host "[FAIL] g++ not found. Please install MinGW and add it to PATH." -ForegroundColor Red
    Pause; exit 1
}
Write-Host "[OK] g++ found" -ForegroundColor Green

# 3. CMake configure if needed
if (-not (Test-Path "$buildDir\Makefile")) {
    Write-Host ""
    Write-Host "[STEP] Running cmake configure..." -ForegroundColor Cyan
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    Push-Location $buildDir
    try {
        $qtPath = if ($env:CMAKE_PREFIX_PATH) { $env:CMAKE_PREFIX_PATH } else { "C:/msys64/mingw64" }
        cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$qtPath"
        if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
    } finally {
        Pop-Location
    }
    Write-Host "[OK] cmake configure done" -ForegroundColor Green
}

# 4. Build
Write-Host ""
Write-Host "[STEP] Building, please wait..." -ForegroundColor Cyan
Push-Location $buildDir
try {
    cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
} finally {
    Pop-Location
}
Write-Host "[OK] Build completed" -ForegroundColor Green
Write-Host ""

# 5. Launch
Write-Host "[RUN] Starting application..." -ForegroundColor Cyan
Start-Process $exePath
Write-Host "Done. You may close this window." -ForegroundColor Green
Start-Sleep -Seconds 2
