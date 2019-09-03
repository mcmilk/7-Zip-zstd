@echo off

set ROOT=%cd%\7zip
if not defined OUTDIR set OUTDIR=%ROOT%\bin64
if not defined ERRFILE set ERRFILE=%cd%\error.txt
if not defined SUBSYS set SUBSYS="5.01"
mkdir %OUTDIR%

set OPTS=PLATFORM=x64 MY_STATIC_LINK=1 /NOLOGO
set LFLAGS=/SUBSYSTEM:WINDOWS,%SUBSYS%

cd %ROOT%\Bundles\Format7zExtract
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7zxa.dll" >> %ERRFILE%
copy X64\7zxa.dll %OUTDIR%\7zxa.dll

cd %ROOT%\Bundles\Format7z
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7za.dll" >> %ERRFILE%
copy X64\7za.dll %OUTDIR%\7za.dll

cd %ROOT%\Bundles\Format7zF
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7z.dll" >> %ERRFILE%
copy X64\7z.dll %OUTDIR%\7z.dll

cd %ROOT%\UI\FileManager
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7zFM.exe" >> %ERRFILE%
copy X64\7zFM.exe %OUTDIR%\7zFM.exe

cd %ROOT%\UI\GUI
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7zG.exe" >> %ERRFILE%
copy X64\7zG.exe %OUTDIR%\7zG.exe

cd %ROOT%\UI\Explorer
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7-zip.dll" >> %ERRFILE%
copy X64\7-zip.dll %OUTDIR%\7-zip.dll

cd %ROOT%\Bundles\SFXWin
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7z.sfx" >> %ERRFILE%
copy X64\7z.sfx %OUTDIR%\7z.sfx

cd %ROOT%\Bundles\Codec_brotli
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ brotli-x64.dll" >> %ERRFILE%
copy X64\brotli.dll %OUTDIR%\brotli-x64.dll

cd %ROOT%\Bundles\Codec_lizard
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ lizard-x64.dll" >> %ERRFILE%
copy X64\lizard.dll %OUTDIR%\lizard-x64.dll

cd %ROOT%\Bundles\Codec_lz4
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ lz4-x64.dll" >> %ERRFILE%
copy X64\lz4.dll %OUTDIR%\lz4-x64.dll

cd %ROOT%\Bundles\Codec_lz5
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ lz5-x64.dll" >> %ERRFILE%
copy X64\lz5.dll %OUTDIR%\lz5-x64.dll

cd %ROOT%\Bundles\Codec_zstd
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ zstd-x64.dll" >> %ERRFILE%
copy X64\zstd.dll %OUTDIR%\zstd-x64.dll

cd %ROOT%\Bundles\Codec_flzma2
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ flzma2-x64.dll" >> %ERRFILE%
copy X64\flzma2.dll %OUTDIR%\flzma2-x64.dll

cd %ROOT%\..\..\C\Util\7zipInstall
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ Install-x64.exe" >> %ERRFILE%
copy X64\7zipInstall.exe %OUTDIR%\Install-x64.exe

cd %ROOT%\..\..\C\Util\7zipUninstall
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ Uninstall.exe" >> %ERRFILE%
copy X64\7zipUninstall.exe %OUTDIR%\Uninstall.exe

set LFLAGS=/SUBSYSTEM:CONSOLE,%SUBSYS%
cd %ROOT%\UI\Console
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7z.exe" >> %ERRFILE%
copy X64\7z.exe %OUTDIR%\7z.exe

cd %ROOT%\Bundles\SFXCon
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7zCon.sfx" >> %ERRFILE%
copy X64\7zCon.sfx %OUTDIR%\7zCon.sfx

cd %ROOT%\Bundles\Alone
nmake %OPTS%
IF %errorlevel% NEQ 0 echo "Error x64 @ 7za.exe" >> %ERRFILE%
copy X64\7za.exe %OUTDIR%\7za.exe

:ende
cd %ROOT%\..
