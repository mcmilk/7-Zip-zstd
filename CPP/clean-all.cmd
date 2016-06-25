@echo off

set ROOT=Z:\projekte\7zip-ZStd\git\CPP\7zip
set OPTS=clean

cd %ROOT%\Bundles\Format7zF
nmake %OPTS%

cd %ROOT%\Bundles\Format7zZStd
nmake %OPTS%

cd %ROOT%\Bundles\SFXCon
nmake %OPTS%

cd %ROOT%\Bundles\SFXWin
nmake %OPTS%

cd %ROOT%\UI\Console
nmake %OPTS%

cd %ROOT%\UI\FileManager
nmake %OPTS%

cd %ROOT%\UI\GUI
nmake %OPTS%

cd %ROOT%\..\..\C\Util\7zipInstall
nmake %OPTS%

cd %ROOT%\..\..\C\Util\7zipUninstall
nmake %OPTS%

rem 64bit
set OPTS=CPU=AMD64 clean

cd %ROOT%\Bundles\Format7zF
nmake %OPTS%

cd %ROOT%\Bundles\Format7zZStd
nmake %OPTS%

cd %ROOT%\Bundles\SFXCon
nmake %OPTS%

cd %ROOT%\Bundles\SFXWin
nmake %OPTS%

cd %ROOT%\UI\Console
nmake %OPTS%

cd %ROOT%\UI\FileManager
nmake %OPTS%

cd %ROOT%\UI\GUI
nmake %OPTS%

cd %ROOT%\..\..\C\Util\7zipInstall
nmake %OPTS%

cd %ROOT%\..\..\C\Util\7zipUninstall
nmake %OPTS%

cd %ROOT%\..
