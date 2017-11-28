@echo off

REM Microsoft Windows SDK 7.1    (VC=sdk71)
REM Microsoft Visual Studio 2012 (VC=11.0)
REM Microsoft Visual Studio 2013 (VC=12.0)
REM Microsoft Visual Studio 2015 (VC=14.0)

REM to many vcvarsall.cmd calls will blow it up!
set OPATH=%PATH%
set ERRFILE=%APPVEYOR_BUILD_FOLDER%\error.txt
cd %APPVEYOR_BUILD_FOLDER%\CPP

REM I am using sdk71 and 14.0 for releases... /TR

:sdk71
set VC=sdk71
set NEXT=vc14
goto build_sdk

:vc11
set VC=11.0
set NEXT=vc12
goto build

:vc12
set VC=12.0
set CFLAGS=-Gw
set NEXT=vc14
goto build

:vc14
set VC=14.0
set NEXT=end
goto build

:build
FOR /R .\ %%d IN (AMD64 O) DO rd /S /Q %%d 2>NUL
set PATH=%OPATH%
call "C:\Program Files (x86)\Microsoft Visual Studio %VC%\VC\vcvarsall.bat" x86
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-x32
echo Building x32
call build-x32.cmd 1>%APPVEYOR_BUILD_FOLDER%\build-%VC%-x32.txt 2>&1
call "C:\Program Files (x86)\Microsoft Visual Studio %VC%\VC\vcvarsall.bat" x86_amd64
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-x64
echo Building x64
call build-x64.cmd 1>%APPVEYOR_BUILD_FOLDER%\build-%VC%-x64.txt 2>&1
goto %NEXT%

:build_sdk
set PATH=%OPATH%
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /Release /ia64 /xp
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-ia64
echo Building ia64
call build-ia64.cmd 1>%APPVEYOR_BUILD_FOLDER%\build-%VC%-ia64.txt 2>&1
goto %NEXT%

:end
cd %APPVEYOR_BUILD_FOLDER%
set > env.txt
7z a %APPVEYOR_PROJECT_NAME%-%APPVEYOR_BUILD_VERSION%.7z bin-* *.txt

rem IF EXIST error.txt exit 1
exit 0
