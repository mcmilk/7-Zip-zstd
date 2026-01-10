@echo off
REM Build some release of 7-Zip ZS

SET COPYCMD=/Y /B
SET COPTS=-m0=lzma -mx9 -ms=on -mf=bcj2
SET URL=https://www.7-zip.org/a/7z2501.exe
SET VERSION=25.01
SET SZIP="C:\Program Files\7-Zip\7z.exe"
SET LURL=https://raw.githubusercontent.com/mcmilk/7-Zip-zstd/master/CPP/7zip/Bundles

SET WD=%cd%
SET SKEL=%WD%\skel

REM Download our skeleton files
mkdir %SKEL%
cd %SKEL%
curl %URL% -L -o 7-Zip.exe
%SZIP% x 7-Zip.exe
del 7-Zip.exe
goto start

@rem doit function ...
:doit
SET ARCH=%~1
SET ZIP32=%~2
SET BIN=%~3
echo Doing %ARCH% in SOURCE=%BIN%

REM 7-Zip Files
cd %SKEL%
del *.exe *.dll *.sfx
FOR %%f IN (7z.dll 7z.exe 7z.sfx 7za.dll 7za.exe 7zCon.sfx 7zFM.exe 7zG.exe 7-zip.dll 7zxa.dll Uninstall.exe) DO (
  copy %BIN%\%%f %%f
)
IF NOT "%ZIP32%" == "" copy %ZIP32% 7-zip32.dll
%SZIP% a ..\%ARCH%.7z %COPTS%
cd %WD%
copy %BIN%\Install.exe + %ARCH%.7z 7z%VERSION%-zstd-%ARCH%.exe
del %ARCH%.7z

REM Codec Files
mkdir codecs-%ARCH%
FOR %%f IN (brotli flzma2 lizard lz4 lz5 zstd) DO (
  copy %BIN%\%%f.dll codecs-%ARCH%\%%f.dll
)
cd codecs-%ARCH%
curl %LURL%/Codecs/LICENSE --output LICENSE
curl %LURL%/Codecs/README.md --output README.md
%SZIP% a ..\Codecs-%ARCH%.7z %COPTS%
cd %WD% && rd /S /Q Codecs-%ARCH%

goto :eof
@rem end of doit function.

REM Currently we build 3 architectures as 6 targets with and without darkmode (ndm suffix)
:start

call :doit x32       ""                            "%WD%\bin-x86"
call :doit x32-ndm   ""                            "%WD%\bin-x86-ndm"

call :doit x64       "%WD%\bin-x86\7-zip.dll"      "%WD%\bin-x64"
call :doit x64-ndm   "%WD%\bin-x86-ndm\7-zip.dll"  "%WD%\bin-x64-ndm"

call :doit arm64     "%WD%\bin-arm\7-zip.dll"      "%WD%\bin-arm64"
call :doit arm64-ndm "%WD%\bin-arm-ndm\7-zip.dll"  "%WD%\bin-arm64-ndm"

REM cleanup
cd %WD%
rd /S /Q %SKEL%
