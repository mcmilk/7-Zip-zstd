@echo off

REM Microsoft Windows SDK 7.1    (VC=sdk71)
REM Microsoft Visual Studio 2010 (VC=10.0)
REM Microsoft Visual Studio 2012 (VC=11.0)
REM Microsoft Visual Studio 2013 (VC=12.0)
REM Microsoft Visual Studio 2015 (VC=14.0)
REM Microsoft Visual Studio 2017 (VC=15.0)

REM to many vcvarsall.cmd calls will blow it up!
set OPATH=%PATH%
set ERRFILE=%APPVEYOR_BUILD_FOLDER%\error.txt
cd %APPVEYOR_BUILD_FOLDER%\CPP

REM I am using sdk71 and 12.0 for releases... /TR

:sdk71
set VC=sdk71
set NEXT=vc12
goto build_sdk

:vc11
set VC=11.0
set NEXT=vc12
goto build

:vc12
set VC=12.0
set CFLAGS=-Gw
set NEXT=end
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
call build-x32.cmd
call "C:\Program Files (x86)\Microsoft Visual Studio %VC%\VC\vcvarsall.bat" x86_amd64
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-x64
call build-x64.cmd
goto %NEXT%

:build_sdk
set PATH=%OPATH%
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
REM call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /Release /x86 /xp
REM set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-x32
REM call build-x32.cmd
REM call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /Release /x64 /xp
REM set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-x64
REM call build-x64.cmd
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /Release /ia64 /xp
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-ia64
call build-ia64.cmd
goto %NEXT%

:end
cd %APPVEYOR_BUILD_FOLDER%
set > env.txt
7z a %APPVEYOR_PROJECT_NAME%-%APPVEYOR_BUILD_VERSION%.7z bin-* *.txt

