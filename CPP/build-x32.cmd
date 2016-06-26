@echo on

set ROOT=Z:\projekte\7zip-ZStd\git\CPP\7zip
set OPTS=MY_STATIC_LINK=1
set LFLAGS=/SUBSYSTEM:WINDOWS,"5.01"

mkdir %ROOT%\bin32

cd %ROOT%\Bundles\Format7zF
nmake %OPTS%
copy O\7z.dll %ROOT%\bin32\7z.dll

cd %ROOT%\Bundles\Format7zZStd
nmake %OPTS%
copy O\7z.dll %ROOT%\bin32\7za.dll

cd %ROOT%\UI\FileManager
nmake %OPTS%
copy O\7zFM.exe %ROOT%\bin32\7zFM.exe

cd %ROOT%\UI\GUI
nmake %OPTS%
copy O\7zG.exe %ROOT%\bin32\7zG.exe

cd %ROOT%\Bundles\SFXWin
nmake %OPTS%
copy O\7z.sfx %ROOT%\bin32\7z.sfx

cd %ROOT%\..\..\C\Util\7zipInstall
nmake %OPTS%
copy O\7zipInstall.exe %ROOT%\bin32\Install-x32.exe

cd %ROOT%\..\..\C\Util\7zipUninstall
nmake %OPTS%
copy O\7zipUninstall.exe %ROOT%\bin32\Uninstall.exe

set LFLAGS=/SUBSYSTEM:CONSOLE,"5.01"
cd %ROOT%\UI\Console
nmake %OPTS%
copy O\7z.exe %ROOT%\bin32\7z.exe

cd %ROOT%\Bundles\SFXCon
nmake %OPTS%
copy O\7zCon.sfx %ROOT%\bin32\7zCon.sfx

:ende
cd %ROOT%\..
