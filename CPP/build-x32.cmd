@echo on

set ROOT=Z:\projekte\7zip-ZStd\git\CPP\7zip
set OUTDIR=%ROOT%\bin32
mkdir %OUTDIR%

set OPTS=MY_STATIC_LINK=1
set LFLAGS=/SUBSYSTEM:WINDOWS,"5.01"

cd %ROOT%\Bundles\Format7z
nmake %OPTS%
copy O\7za.dll %OUTDIR%\7za.dll

cd %ROOT%\Bundles\Format7zF
nmake %OPTS%
copy O\7z.dll %OUTDIR%\7z.dll

cd %ROOT%\UI\FileManager
nmake %OPTS%
copy O\7zFM.exe %OUTDIR%\7zFM.exe

cd %ROOT%\UI\GUI
nmake %OPTS%
copy O\7zG.exe %OUTDIR%\7zG.exe

cd %ROOT%\Bundles\SFXWin
nmake %OPTS%
copy O\7z.sfx %OUTDIR%\7z.sfx

cd %ROOT%\Bundles\Codec_zstd
nmake %OPTS%
copy O\zstd.dll %OUTDIR%\zstd-x32.dll

cd %ROOT%\Bundles\Codec_zstdF
nmake %OPTS%
copy O\zstd.dll %OUTDIR%\zstd-x32big.dll

cd %ROOT%\..\..\C\Util\7zipInstall
nmake %OPTS%
copy O\7zipInstall.exe %OUTDIR%\Install-x32.exe

cd %ROOT%\..\..\C\Util\7zipUninstall
nmake %OPTS%
copy O\7zipUninstall.exe %OUTDIR%\Uninstall.exe

set LFLAGS=/SUBSYSTEM:CONSOLE,"5.01"
cd %ROOT%\UI\Console
nmake %OPTS%
copy O\7z.exe %OUTDIR%\7z.exe

cd %ROOT%\Bundles\SFXCon
nmake %OPTS%
copy O\7zCon.sfx %OUTDIR%\7zCon.sfx

:ende
cd %ROOT%\..
