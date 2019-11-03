@echo off

REM Microsoft Windows SDK 7.1    (VC=sdk71) -> can compile for IA64, but who needs that?
REM Microsoft Visual Studio 2010 (VC=10.0) -> for win2k
REM Microsoft Visual Studio 2012 (VC=11.0)
REM Microsoft Visual Studio 2013 (VC=12.0)
REM Microsoft Visual Studio 2015 (VC=14.0)
REM Microsoft Visual Studio 2017 (VC=15.0)  -> for: x32 + x64

REM to many vcvarsall.cmd calls will blow it up!
set OPATH=%PATH%
set ERRFILE=%APPVEYOR_BUILD_FOLDER%\error.txt
cd %APPVEYOR_BUILD_FOLDER%\CPP

REM releases now:
REM sdk71: ia builds
REM vc10: win2k builds
REM vc14: >= winxp builds
REM /TR 2019-09-07
goto vc14

:sdk71
set VC=sdk71
set NEXT=vc10
goto build_sdk

:vc10
set VC=10.0
set NEXT=vc14
set CFLAGS=
goto build_vc

:vc11
set VC=11.0
set NEXT=end
set CFLAGS=-Gw
goto build_vc

:vc12
set VC=12.0
set NEXT=end
set CFLAGS=-Gw
goto build_vc

:vc14
set VC=14.0
set NEXT=end
set CFLAGS=-Gw
goto build_vc


:build_sdk
set PATH=%OPATH%
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /Release /ia64 /xp
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-ia64
call build-ia64.cmd
goto %NEXT%

:build_vc
FOR /R .\ %%d IN (ARM X64 O) DO rd /S /Q %%d 2>NUL
set PATH=%OPATH%
call "C:\Program Files (x86)\Microsoft Visual Studio %VC%\VC\vcvarsall.bat" x86
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-x32
call build-x32.cmd
set PATH=%OPATH%
call "C:\Program Files (x86)\Microsoft Visual Studio %VC%\VC\vcvarsall.bat" x86_amd64
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-x64
call build-x64.cmd
set PATH=%OPATH%
call "C:\Program Files (x86)\Microsoft Visual Studio %VC%\VC\vcvarsall.bat" x86_arm
set OUTDIR=%APPVEYOR_BUILD_FOLDER%\bin-%VC%-arm
call build-arm.cmd
goto %NEXT%

:end
cd %APPVEYOR_BUILD_FOLDER%
set > env.txt
7z a %APPVEYOR_PROJECT_NAME%-%APPVEYOR_BUILD_VERSION%.7z bin-* *.txt
