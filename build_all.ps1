param(
    [string[]]$Platforms = @("x64", "x86"),
    [string]$OutputDir,
    [switch]$SkipCleanup,
    [switch]$BuildMsi
)

$ErrorActionPreference = "Stop"

$RootDir = $PSScriptRoot
if (-not $OutputDir) {
    $OutputDir = Join-Path $RootDir "build"
}

# Auto-detect VS installation path
$VcVarsPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
)

$Vcvarsall = $null
foreach ($path in $VcVarsPaths) {
    if (Test-Path $path) {
        $Vcvarsall = $path
        break
    }
}

if (-not $Vcvarsall) {
    Write-Host "Error: Visual Studio vcvarsall.bat not found!" -ForegroundColor Red
    Write-Host "Please install Visual Studio 2022/2019 with C++ tools." -ForegroundColor Yellow
    exit 1
}

Write-Host "Found VS environment: $Vcvarsall" -ForegroundColor Green

function Invoke-EnvSetup {
    param([string]$Platform)

    Write-Host "Setting up VS environment for $Platform..." -ForegroundColor Cyan
    $vcEnv = cmd /c "`"$Vcvarsall`" $Platform && set" 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to set up VS environment for $Platform"
    }
    $vcEnv | ForEach-Object {
        if ($_ -match "^([^=]+)=(.*)$") {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }
}

function Invoke-Nmake {
    param(
        [string]$ComponentName,
        [string]$BuildDir,
        [string]$MakefileDir,
        [string]$Makefile = "makefile",
        [string]$Platform
    )
    $cacheDir = Join-Path $BuildDir "cache\$Platform\$ComponentName"
    if (-not (Test-Path $cacheDir)) {
        New-Item -ItemType Directory -Path $cacheDir -Force | Out-Null
    }

    Push-Location (Join-Path $RootDir $MakefileDir)
    try {
        Write-Host "Building $ComponentName ($Platform)..." -ForegroundColor Cyan
        & nmake /f $Makefile PLATFORM=$Platform O="$cacheDir"
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to build $ComponentName"
        }
        Write-Host "  $ComponentName built successfully" -ForegroundColor Green
    } finally {
        Pop-Location
    }
}

function Get-ProgName {
    param([string]$ComponentName)
    $progs = @{
        "Format7zF"    = "7z.dll"
        "Fm"           = "7zFM.exe"
        "SFXWin"       = "7z.sfx"
        "SFXCon"       = "7zCon.sfx"
        "GUI"          = "7zG.exe"
        "Console"      = "7z.exe"
        "Explorer"     = "7-zip.dll"
        "SfxSetup"     = "7zS2.sfx"
        "7zipInstall"  = "7zipInstall.exe"
        "7zipUninstall" = "7zipUninstall.exe"
    }
    return $progs[$ComponentName]
}

$Components = @(
    @{Name="Format7zF";    Path="CPP\7zip\Bundles\Format7zF"},
    @{Name="Fm";           Path="CPP\7zip\Bundles\Fm"},
    @{Name="SFXWin";       Path="CPP\7zip\Bundles\SFXWin"},
    @{Name="SFXCon";       Path="CPP\7zip\Bundles\SFXCon"},
    @{Name="GUI";          Path="CPP\7zip\UI\GUI"},
    @{Name="Console";      Path="CPP\7zip\UI\Console"},
    @{Name="Explorer";     Path="CPP\7zip\UI\Explorer"},
    @{Name="SfxSetup";     Path="C\Util\SfxSetup"},
    @{Name="7zipInstall";  Path="C\Util\7zipInstall"},
    @{Name="7zipUninstall";Path="C\Util\7zipUninstall"}
)

foreach ($Platform in $Platforms) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host "Building for $Platform..." -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow

    $PlatformDir = Join-Path $OutputDir $Platform
    if (-not (Test-Path $PlatformDir)) {
        New-Item -ItemType Directory -Path $PlatformDir -Force | Out-Null
    }

    Invoke-EnvSetup -Platform $Platform

    foreach ($Component in $Components) {
        $prog = Get-ProgName -ComponentName $Component.Name
        $progPath = Join-Path $PlatformDir $prog
        if (Test-Path $progPath) {
            Write-Host "  $prog already exists, skipping..." -ForegroundColor Gray
            continue
        }

        Invoke-Nmake -ComponentName $Component.Name -BuildDir $OutputDir -MakefileDir $Component.Path -Platform $Platform

        $cacheDir = Join-Path $OutputDir "cache\$Platform\$($Component.Name)"
        $builtProg = Join-Path $cacheDir $prog
        if (Test-Path $builtProg) {
            Copy-Item $builtProg $PlatformDir -Force
            Write-Host "  Copied $prog to $PlatformDir" -ForegroundColor Green
        } else {
            Write-Host "  Warning: $prog not found at $builtProg" -ForegroundColor Yellow
        }
    }

    Write-Host ""
    Write-Host "Files in ${PlatformDir}:" -ForegroundColor Cyan
    Get-ChildItem $PlatformDir -File | ForEach-Object {
        $size = [math]::Round($_.Length/1KB, 1)
        Write-Host "  $($_.Name) - $size KB"
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Yellow
Write-Host "Creating portable zip packages..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow

$7zPath = Get-Command "7z" -ErrorAction SilentlyContinue
if (-not $7zPath) { $7zPath = Get-Command "7za" -ErrorAction SilentlyContinue }

if ($7zPath) {
    $version = "26.01"
    $verFile = Join-Path $RootDir "DOC\src-history.txt"
    if (Test-Path $verFile) {
        $firstLine = Get-Content $verFile -TotalCount 1
        if ($firstLine -match "(\d+\.\d+)") {
            $version = $matches[1]
        }
    }

    foreach ($Platform in $Platforms) {
        $PlatformDir = Join-Path $OutputDir $Platform
        $zipName = "7zip-$version-$Platform.7z"
        $zipPath = Join-Path $OutputDir $zipName
        if (Test-Path $zipPath) {
            Remove-Item $zipPath -Force
        }
        & $7zPath.Path a -mx=9 $zipPath (Join-Path $PlatformDir "*")
        Write-Host "  Created: $zipName" -ForegroundColor Green
    }

    # Create combined portable zip with both platforms
    $combinedZip = "7zip-$version-portable.7z"
    $combinedPath = Join-Path $OutputDir $combinedZip
    if (Test-Path $combinedPath) {
        Remove-Item $combinedPath -Force
    }
    Push-Location $OutputDir
    try {
        $files = @()
        foreach ($Platform in $Platforms) {
            $dir = Join-Path $OutputDir $Platform
            $files += Get-ChildItem $dir -File | ForEach-Object { "$Platform\$($_.Name)" }
        }
        & $7zPath.Path a -mx=9 $combinedPath @files
        Write-Host "  Created: $combinedZip" -ForegroundColor Green
    } finally {
        Pop-Location
    }
} else {
    Write-Host "  7z not found, skipping portable zip creation" -ForegroundColor Yellow
    Write-Host "  Install 7-Zip or p7zip to enable zip packaging" -ForegroundColor Yellow
}

if ($BuildMsi) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host "Building MSI installer..." -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow

    $wixTool = Get-Command "wix" -ErrorAction SilentlyContinue
    if (-not $wixTool) {
        Write-Host "WiX Toolset not found. Attempting to install..." -ForegroundColor Yellow
        try {
            dotnet tool install --global wix --version 5.0.2 2>$null
            $wixTool = Get-Command "wix" -ErrorAction SilentlyContinue
        } catch {
            Write-Host "Failed to install WiX Toolset" -ForegroundColor Red
        }
    }

    if ($wixTool) {
        $wxsFile = Join-Path $RootDir "DOC\7zip.wxs"
        if (Test-Path $wxsFile) {
            $msiDir = Join-Path $OutputDir "msi"
            if (-not (Test-Path $msiDir)) {
                New-Item -ItemType Directory -Path $msiDir -Force | Out-Null
            }

            $stagingDir = Join-Path $OutputDir "msi\staging"
            if (Test-Path $stagingDir) {
                Remove-Item $stagingDir -Recurse -Force
            }
            New-Item -ItemType Directory -Path $stagingDir -Force | Out-Null

            # Copy x64 files to staging
            $x64Dir = Join-Path $OutputDir "x64"
            if (Test-Path $x64Dir) {
                Get-ChildItem $x64Dir -File | ForEach-Object {
                    Copy-Item $_.FullName (Join-Path $stagingDir $_.Name) -Force
                }
            }

            # For x64 build, also copy x86 shell extension as 7-zip32.dll
            $x86Dir = Join-Path $OutputDir "x86"
            $shell32Path = Join-Path $x86Dir "7-zip.dll"
            if ((Test-Path $x64Dir) -and (Test-Path $shell32Path)) {
                Copy-Item $shell32Path (Join-Path $stagingDir "7-zip32.dll") -Force
                Write-Host "  Copied 7-zip32.dll (x86 shell extension) for x64 installer" -ForegroundColor Green
            }

            # Copy DOC files (readme, license, etc.)
            $docFiles = @("License.txt", "readme.txt", "History.txt")
            foreach ($docFile in $docFiles) {
                $src = Join-Path $RootDir "DOC\$docFile"
                if (Test-Path $src) {
                    Copy-Item $src (Join-Path $stagingDir $docFile) -Force
                }
            }

            # Create Lang directory if it doesn't exist
            $langDir = Join-Path $RootDir "CPP\7zip\UI\FileManager\Lang"
            $stagingLangDir = Join-Path $stagingDir "Lang"
            if (Test-Path $langDir) {
                if (-not (Test-Path $stagingLangDir)) {
                    New-Item -ItemType Directory -Path $stagingLangDir -Force | Out-Null
                }
                Get-ChildItem $langDir -Filter "*.txt" -File | ForEach-Object {
                    Copy-Item $_.FullName (Join-Path $stagingLangDir $_.Name) -Force
                }
                Get-ChildItem $langDir -Filter "*.ttt" -File | ForEach-Object {
                    Copy-Item $_.FullName (Join-Path $stagingLangDir $_.Name) -Force
                }
                Write-Host "  Copied $((Get-ChildItem $stagingLangDir).Count) language files" -ForegroundColor Green
            } else {
                Write-Host "  Warning: Lang directory not found at $langDir" -ForegroundColor Yellow
                Write-Host "  MSI will be built without language files" -ForegroundColor Yellow
            }

            # Copy help file
            $chmFile = Join-Path $RootDir "DOC\7-zip.chm"
            if (Test-Path $chmFile) {
                Copy-Item $chmFile (Join-Path $stagingDir "7-zip.chm") -Force
            }

            # Build MSI
            $msiOutput = Join-Path $msiDir "7zip-$version-x64.msi"
            Write-Host "Building MSI: $msiOutput" -ForegroundColor Cyan

            Push-Location $stagingDir
            try {
                # wix build with bind path pointing to staging
                & wix build $wxsFile -bind-path . -o $msiOutput -arch x64
                if ($LASTEXITCODE -eq 0) {
                    Write-Host "  MSI created: $msiOutput" -ForegroundColor Green
                } else {
                    Write-Host "  MSI build failed" -ForegroundColor Red
                }
            } catch {
                Write-Host "  MSI build error: $_" -ForegroundColor Red
            } finally {
                Pop-Location
            }

            # Cleanup staging
            Remove-Item $stagingDir -Recurse -Force
        } else {
            Write-Host "  WiX source file not found: $wxsFile" -ForegroundColor Red
        }
    } else {
        Write-Host "  WiX Toolset not available, skipping MSI build" -ForegroundColor Yellow
    }
}

if (-not $SkipCleanup) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Yellow
    Write-Host "Cleaning up build cache..." -ForegroundColor Yellow
    Write-Host "========================================" -ForegroundColor Yellow

    $cachePath = Join-Path $OutputDir "cache"
    if (Test-Path $cachePath) {
        Remove-Item -Path $cachePath -Recurse -Force
        Write-Host "  Removed: $cachePath" -ForegroundColor Gray
    }

    $msiCachePath = Join-Path $OutputDir "msi"
    if (Test-Path $msiCachePath) {
        Remove-Item -Path $msiCachePath -Recurse -Force
        Write-Host "  Removed: $msiCachePath" -ForegroundColor Gray
    }

    Write-Host "  Build cache cleaned." -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "Output files in: $OutputDir" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

Get-ChildItem $OutputDir -Directory | Where-Object { $_.Name -ne "cache" -and $_.Name -ne "msi" } | ForEach-Object {
    Get-ChildItem $_.FullName -File | ForEach-Object {
        $size = [math]::Round($_.Length/1KB, 1)
        $relativePath = $_.FullName.Substring($OutputDir.Length + 1)
        Write-Host "  $relativePath - $size KB"
    }
}

Get-ChildItem $OutputDir -Filter "*.7z" -File | ForEach-Object {
    $size = [math]::Round($_.Length/1KB, 1)
    Write-Host "  $($_.Name) - $size KB"
}

Get-ChildItem $OutputDir -Filter "*.msi" -Recurse -File | ForEach-Object {
    $size = [math]::Round($_.Length/1KB, 1)
    $relativePath = $_.FullName.Substring($OutputDir.Length + 1)
    Write-Host "  $relativePath - $size KB"
}

Read-Host -Prompt "Press Enter to exit"
