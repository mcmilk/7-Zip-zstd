@echo on

set ROOT=Z:\projekte\7zip-ZStd\git\CPP\7zip
set OPTS=CPU=AMD64 MY_STATIC_LINK=1
set LFLAGS=/SUBSYSTEM:WINDOWS,"5.02"

mkdir %ROOT%\bin64

cd %ROOT%\Bundles\Format7zF
nmake %OPTS%
copy AMD64\7z.dll %ROOT%\bin64\7z.dll

cd %ROOT%\Bundles\Format7zZStd
nmake %OPTS%
copy AMD64\7z.dll %ROOT%\bin64\7za.dll

cd %ROOT%\UI\FileManager
nmake %OPTS%
copy AMD64\7zFM.exe %ROOT%\bin64\7zFM.exe

cd %ROOT%\UI\GUI
nmake %OPTS%
copy AMD64\7zG.exe %ROOT%\bin64\7zG.exe

cd %ROOT%\Bundles\SFXWin
nmake %OPTS%
copy AMD64\7z.sfx %ROOT%\bin64\7z.sfx

cd %ROOT%\..\..\C\Util\7zipInstall
nmake %OPTS%
copy AMD64\7zipInstall.exe %ROOT%\bin64\Install-x64.exe

cd %ROOT%\..\..\C\Util\7zipUninstall
nmake %OPTS%
copy AMD64\7zipUninstall.exe %ROOT%\bin64\Uninstall.exe

set LFLAGS=/SUBSYSTEM:CONSOLE,"5.02"
cd %ROOT%\UI\Console
nmake %OPTS%
copy AMD64\7z.exe %ROOT%\bin64\7z.exe

cd %ROOT%\Bundles\SFXCon
nmake %OPTS%
copy AMD64\7zCon.sfx %ROOT%\bin64\7zCon.sfx

cd %ROOT%\..
