@echo on

set ROOT=%cd%\7zip
if not defined OUTDIR set OUTDIR=%ROOT%\binIA64
mkdir %OUTDIR%

set OPTS=CPU=IA64 MY_STATIC_LINK=1
set LFLAGS=/SUBSYSTEM:WINDOWS,"5.02"

cd %ROOT%\Bundles\Format7zExtract
nmake %OPTS%
copy IA64\7zxa.dll %OUTDIR%\7zxa.dll

cd %ROOT%\Bundles\Format7z
nmake %OPTS%
copy IA64\7za.dll %OUTDIR%\7za.dll

cd %ROOT%\Bundles\Format7zF
nmake %OPTS%
copy IA64\7z.dll %OUTDIR%\7z.dll

cd %ROOT%\Bundles\Format7zFO
nmake %OPTS%
copy IA64\7z.dll %OUTDIR%\7zOrig.dll

cd %ROOT%\Bundles\Format7zUSB
nmake %OPTS%
copy IA64\7zu.dll %OUTDIR%\7zu-x64.dll

cd %ROOT%\UI\FileManager
nmake %OPTS%
copy IA64\7zFM.exe %OUTDIR%\7zFM.exe

cd %ROOT%\UI\GUI
nmake %OPTS%
copy IA64\7zG.exe %OUTDIR%\7zG.exe

cd %ROOT%\UI\Explorer
nmake %OPTS%
copy IA64\7-zip.dll %OUTDIR%\7-zip.dll

cd %ROOT%\Bundles\SFXWin
nmake %OPTS%
copy IA64\7z.sfx %OUTDIR%\7z.sfx

cd %ROOT%\Bundles\Codec_brotli
nmake %OPTS%
copy IA64\brotli.dll %OUTDIR%\brotli-x64.dll

cd %ROOT%\Bundles\Codec_lizard
nmake %OPTS%
copy IA64\lizard.dll %OUTDIR%\lizard-x64.dll

cd %ROOT%\Bundles\Codec_lz4
nmake %OPTS%
copy IA64\lz4.dll %OUTDIR%\lz4-x64.dll

cd %ROOT%\Bundles\Codec_lz5
nmake %OPTS%
copy IA64\lz5.dll %OUTDIR%\lz5-x64.dll

cd %ROOT%\Bundles\Codec_zstdF
nmake %OPTS%
copy IA64\zstd.dll %OUTDIR%\zstd-x64.dll

cd %ROOT%\..\..\C\Util\7zipInstall
nmake %OPTS%
copy IA64\7zipInstall.exe %OUTDIR%\Install-x64.exe

cd %ROOT%\..\..\C\Util\7zipUninstall
nmake %OPTS%
copy IA64\7zipUninstall.exe %OUTDIR%\Uninstall.exe

set LFLAGS=/SUBSYSTEM:CONSOLE,"5.02"
cd %ROOT%\UI\Console
nmake %OPTS%
copy IA64\7z.exe %OUTDIR%\7z.exe

cd %ROOT%\Bundles\SFXCon
nmake %OPTS%
copy IA64\7zCon.sfx %OUTDIR%\7zCon.sfx

cd %ROOT%\Bundles\Alone
nmake %OPTS%
copy IA64\7za.exe %OUTDIR%\7za.exe

:ende
cd %ROOT%\..
