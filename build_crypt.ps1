$ErrorActionPreference = "Stop"

$BuildDir = Join-Path $PSScriptRoot "build"
$Vcvarsall = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"

if (-not (Test-Path $Vcvarsall)) {
    Write-Host "Error: vcvarsall.bat not found" -ForegroundColor Red
    exit 1
}

function Invoke-Build {
    param(
        [string]$Platform,
        [string]$OutputDir
    )
    
    Write-Host "Setting up VS environment for $Platform..." -ForegroundColor Cyan
    $vcEnv = cmd /c "`"$Vcvarsall`" $Platform && set" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to set up VS environment" -ForegroundColor Red
        exit 1
    }
    
    $vcEnv | ForEach-Object {
        if ($_ -match "^([^=]+)=(.*)$") {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }
    
    $PlatformDir = Join-Path $OutputDir $Platform
    if (-not (Test-Path $PlatformDir)) {
        New-Item -ItemType Directory -Path $PlatformDir -Force | Out-Null
    }
    
    $cacheDir = Join-Path $OutputDir "cache\$Platform"
    
    $dllCacheDir = Join-Path $cacheDir "Format7zF"
    if (-not (Test-Path $dllCacheDir)) {
        New-Item -ItemType Directory -Path $dllCacheDir -Force | Out-Null
    }
    
    Write-Host ""
    Write-Host "Building 7z.dll ($Platform)..." -ForegroundColor Cyan
    Set-Location (Join-Path $PSScriptRoot "CPP\7zip\Bundles\Format7zF")
    & nmake /f makefile PLATFORM=$Platform O="$dllCacheDir"
    if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; exit 1 }
    Copy-Item "$dllCacheDir\7z.dll" "$PlatformDir\7z.dll" -Force
    Copy-Item "$dllCacheDir\7z.lib" "$PlatformDir\7z.lib" -Force
    
    $guiCacheDir = Join-Path $cacheDir "GUI"
    if (-not (Test-Path $guiCacheDir)) {
        New-Item -ItemType Directory -Path $guiCacheDir -Force | Out-Null
    }
    
    Write-Host ""
    Write-Host "Building 7zG.exe ($Platform)..." -ForegroundColor Cyan
    Set-Location (Join-Path $PSScriptRoot "CPP\7zip\UI\GUI")
    & nmake /f makefile PLATFORM=$Platform O="$guiCacheDir"
    if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; exit 1 }
    Copy-Item "$guiCacheDir\7zG.exe" "$PlatformDir\7zG.exe" -Force
    
    $consoleCacheDir = Join-Path $cacheDir "Console"
    if (-not (Test-Path $consoleCacheDir)) {
        New-Item -ItemType Directory -Path $consoleCacheDir -Force | Out-Null
    }
    
    Write-Host ""
    Write-Host "Building 7z.exe ($Platform)..." -ForegroundColor Cyan
    Set-Location (Join-Path $PSScriptRoot "CPP\7zip\UI\Console")
    & nmake /f makefile PLATFORM=$Platform O="$consoleCacheDir"
    if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; exit 1 }
    Copy-Item "$consoleCacheDir\7z.exe" "$PlatformDir\7z.exe" -Force
    
    $sfxWinCacheDir = Join-Path $cacheDir "SFXWin"
    if (-not (Test-Path $sfxWinCacheDir)) {
        New-Item -ItemType Directory -Path $sfxWinCacheDir -Force | Out-Null
    }
    
    Write-Host ""
    Write-Host "Building 7z.sfx ($Platform)..." -ForegroundColor Cyan
    Set-Location (Join-Path $PSScriptRoot "CPP\7zip\Bundles\SFXWin")
    & nmake /f makefile PLATFORM=$Platform O="$sfxWinCacheDir"
    if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; exit 1 }
    Copy-Item "$sfxWinCacheDir\7z.sfx" "$PlatformDir\7z.sfx" -Force
    
    $sfxConCacheDir = Join-Path $cacheDir "SFXCon"
    if (-not (Test-Path $sfxConCacheDir)) {
        New-Item -ItemType Directory -Path $sfxConCacheDir -Force | Out-Null
    }
    
    Write-Host ""
    Write-Host "Building 7zCon.sfx ($Platform)..." -ForegroundColor Cyan
    Set-Location (Join-Path $PSScriptRoot "CPP\7zip\Bundles\SFXCon")
    & nmake /f makefile PLATFORM=$Platform O="$sfxConCacheDir"
    if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; exit 1 }
    Copy-Item "$sfxConCacheDir\7zCon.sfx" "$PlatformDir\7zCon.sfx" -Force

    $fmCacheDir = Join-Path $cacheDir "Fm"
    if (-not (Test-Path $fmCacheDir)) {
        New-Item -ItemType Directory -Path $fmCacheDir -Force | Out-Null
    }

    Write-Host ""
    Write-Host "Building 7zFM.exe ($Platform)..." -ForegroundColor Cyan
    Set-Location (Join-Path $PSScriptRoot "CPP\7zip\Bundles\Fm")
    & nmake /f makefile PLATFORM=$Platform O="$fmCacheDir"
    if ($LASTEXITCODE -ne 0) { Write-Host "Build failed!" -ForegroundColor Red; exit 1 }
    Copy-Item "$fmCacheDir\7zFM.exe" "$PlatformDir\7zFM.exe" -Force
}

if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

Write-Host "========================================" -ForegroundColor Yellow
Write-Host "Building for x64..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow
Invoke-Build -Platform "x64" -OutputDir $BuildDir

Write-Host ""
Write-Host "========================================" -ForegroundColor Yellow
Write-Host "Building for x86..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow
Invoke-Build -Platform "x86" -OutputDir $BuildDir

Set-Location $PSScriptRoot

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "Output files in: $BuildDir" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

Get-ChildItem $BuildDir -Directory | Where-Object { $_.Name -ne "cache" } | ForEach-Object {
    Get-ChildItem $_.FullName -File | ForEach-Object {
        $size = [math]::Round($_.Length/1KB, 1)
        $relativePath = $_.FullName.Substring($BuildDir.Length + 1)
        Write-Host "  $relativePath - $size KB" 
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Yellow
Write-Host "Cleaning up build cache..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow

$cachePath = Join-Path $BuildDir "cache"
if (Test-Path $cachePath) {
    Remove-Item -Path $cachePath -Recurse -Force
    Write-Host "  Removed: $cachePath" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Build cache cleaned." -ForegroundColor Green

Read-Host -Prompt "Press Enter to exit"
