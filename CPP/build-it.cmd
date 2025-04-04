@echo off

IF not "%~1" == "-no-init" (
  set ROOT=%cd%\7zip
  set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-%PLATFORM%
  set ERRFILE=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-%PLATFORM%.log
  set LFLAGS=/SUBSYSTEM:WINDOWS,%SUBSYS%
  set > %APPVEYOR_BUILD_FOLDER%\env-%VC%-%PLATFORM%.txt
)
if "%SUBSYS%" == "" (
  echo ERROR: Variable SUBSYS is not set.
  exit /b 1
)
if "%STOP_ON_ERROR%" == "" (
  set STOP_ON_ERROR=0
)
echo ** Build %PLATFORM% to %OUTDIR% ...
IF not exist %OUTDIR% mkdir %OUTDIR%

set ERR_COUNT=0

call :build Bundles\Format7zExtract     7zxa.dll
call :build Bundles\Format7z            7za.dll
call :build Bundles\Format7zF           7z.dll
call :build UI\FileManager              7zFM.exe
call :build UI\GUI                      7zG.exe
call :build UI\Explorer                 7-zip.dll
call :build Bundles\SFXWin              7z.sfx
call :build Bundles\Codec_brotli        brotli.dll
call :build Bundles\Codec_lizard        lizard.dll
call :build Bundles\Codec_lz4           lz4.dll
call :build Bundles\Codec_lz5           lz5.dll
call :build Bundles\Codec_zstd          zstd.dll
call :build Bundles\Codec_flzma2        flzma2.dll
call :build ..\..\C\Util\7zipInstall    7zipInstall.exe    Install.exe
call :build ..\..\C\Util\7zipUninstall  7zipUninstall.exe  Uninstall.exe

set LFLAGS=/SUBSYSTEM:CONSOLE,%SUBSYS%

call :build UI\Console                  7z.exe
call :build Bundles\SFXCon              7zCon.sfx
call :build Bundles\Alone               7za.exe

:ende
cd %ROOT%\..
if %ERR_COUNT% NEQ 0 (
  echo !! Build fails with %ERR_COUNT% errors. 
  exit /b 1
)
echo ** Build is OK.
exit /b 0

@rem build function ...
:build
cd %ROOT%\%~1
set out=%~3
if "%out%" == "" set out=%~2
echo   == Build %out% (%~1) ==
nmake /NOLOGO %OPTS%
IF %errorlevel% NEQ 0 (
  IF not "%ERRFILE%" == "" (echo "Error @ %out%" >> "%ERRFILE%")
  set /A ERR_COUNT=ERR_COUNT+1
  IF %STOP_ON_ERROR% NEQ 0 EXIT 1
  exit /b 1
)
copy %PLATFORM%\%~2 %OUTDIR%\%out%
IF %errorlevel% NEQ 0 (
  IF %STOP_ON_ERROR% NEQ 0 EXIT 1
  exit /b 1
)
goto :eof
