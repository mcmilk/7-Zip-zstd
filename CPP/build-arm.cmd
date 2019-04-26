@echo off

set ROOT=%cd%\7zip
if not defined OUTDIR set OUTDIR=%ROOT%\binArm
if not defined ERRFILE set ERRFILE=%cd%\error.txt
mkdir %OUTDIR%

set OPTS=PLATFORM=arm MY_STATIC_LINK=1 /NOLOGO

cd %ROOT%\Bundles\Format7zExtract
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ 7zxa.dll" >> %ERRFILE%
copy arm\7zxa.dll %OUTDIR%\7zxa.dll

cd %ROOT%\Bundles\Format7z
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ 7za.dll" >> %ERRFILE%
copy arm\7za.dll %OUTDIR%\7za.dll

cd %ROOT%\Bundles\Format7zF
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ 7z.dll" >> %ERRFILE%
copy arm\7z.dll %OUTDIR%\7z.dll

REM cd %ROOT%\UI\FileManager
REM nmake %OPTS%
REM IF %errorlevel% NEQ 0 echo "Error arm @ 7zFM.exe" >> %ERRFILE%
REM copy arm\7zFM.exe %OUTDIR%\7zFM.exe

REM cd %ROOT%\UI\GUI
REM nmake %OPTS%
REM IF %errorlevel% NEQ 0 echo "Error arm @ 7zG.exe" >> %ERRFILE%
REM copy arm\7zG.exe %OUTDIR%\7zG.exe

cd %ROOT%\UI\Explorer
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ 7-zip.dll" >> %ERRFILE%
copy arm\7-zip.dll %OUTDIR%\7-zip.dll

REM cd %ROOT%\Bundles\SFXWin
REM nmake %OPTS%
REM IF %errorlevel% NEQ 0 echo "Error arm @ 7z.sfx" >> %ERRFILE%
REM copy arm\7z.sfx %OUTDIR%\7z.sfx

cd %ROOT%\Bundles\Codec_brotli
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ brotli-arm.dll" >> %ERRFILE%
copy arm\brotli.dll %OUTDIR%\brotli-arm.dll

cd %ROOT%\Bundles\Codec_lizard
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ lizard-arm.dll" >> %ERRFILE%
copy arm\lizard.dll %OUTDIR%\lizard-arm.dll

cd %ROOT%\Bundles\Codec_lz4
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ lz4-arm.dll" >> %ERRFILE%
copy arm\lz4.dll %OUTDIR%\lz4-arm.dll

cd %ROOT%\Bundles\Codec_lz5
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ lz5-arm.dll" >> %ERRFILE%
copy arm\lz5.dll %OUTDIR%\lz5-arm.dll

cd %ROOT%\Bundles\Codec_zstd
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ zstd-arm.dll" >> %ERRFILE%
copy arm\zstd.dll %OUTDIR%\zstd-arm.dll

cd %ROOT%\Bundles\Codec_flzma2
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ flzma2-arm.dll" >> %ERRFILE%
copy arm\flzma2.dll %OUTDIR%\flzma2-arm.dll

REM cd %ROOT%\..\..\C\Util\7zipInstall
REM nmake %OPTS%
REM IF %errorlevel% NEQ 0 echo "Error arm @ Install-arm.exe" >> %ERRFILE%
REM copy arm\7zipInstall.exe %OUTDIR%\Install-arm.exe

REM cd %ROOT%\..\..\C\Util\7zipUninstall
REM nmake %OPTS%
REM IF %errorlevel% NEQ 0 echo "Error arm @ Uninstall.exe" >> %ERRFILE%
REM copy arm\7zipUninstall.exe %OUTDIR%\Uninstall.exe

cd %ROOT%\UI\Console
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ 7z.exe" >> %ERRFILE%
copy arm\7z.exe %OUTDIR%\7z.exe

cd %ROOT%\Bundles\SFXCon
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ 7zCon.sfx" >> %ERRFILE%
copy arm\7zCon.sfx %OUTDIR%\7zCon.sfx

cd %ROOT%\Bundles\Alone
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error arm @ 7za.exe" >> %ERRFILE%
copy arm\7za.exe %OUTDIR%\7za.exe

:ende
cd %ROOT%\..
