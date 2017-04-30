@echo off
REM Build signed setups for 7-Zip ZS /TR

SET WD=%cd%
SET ROOT=%cd%\CPP\7zip
SET COPYCMD=/Y /B
SET TSERVER=http://timestamp.globalsign.com/?signature=sha2

cd %ROOT%
rd /S /Q "x32"
rd /S /Q "x64"
rd /S /Q "codecs"
7z x C:\tmp\x32.zip
7z x C:\tmp\x64.zip
7z x C:\tmp\codecs.zip

FOR %%f IN (7-zip.dll 7z.dll 7z.exe 7za.dll 7za.exe 7zG.exe 7zFM.exe 7z.sfx 7zCon.sfx Uninstall.exe) DO (
  copy bin32\%%f x32\%%f
  copy bin64\%%f x64\%%f
)
copy bin32\7-zip.dll x32\7-zip32.dll

FOR %%f IN (lz4 lz5 zstd) DO (
  copy bin32\%%f-x32.dll codecs\%%f-x32.dll
  copy bin64\%%f-x64.dll codecs\%%f-x64.dll
)
del 32.7z 64.7z codecs.7z

signtool.exe sign /v /fd SHA256 /tr %TSERVER% /td sha256 x32\*.exe x32\*.dll x64\*.exe x64\*.dll

cd x32
7z a ../32.7z -m0=lzma -mx9 -ms=on -mf=bcj2

cd ..\x64
7z a ../64.7z -m0=lzma -mx9 -ms=on -mf=bcj2

cd ..\codecs
7z a ..\Codecs.7z -m0=lzma -mx9 -ms=on -mf=bcj2
cd ..

copy bin32\Install-x32.exe + 32.7z 7z1700-zstd-x32.exe
copy bin64\Install-x64.exe + 64.7z 7z1700-zstd-x64.exe
del 32.7z 64.7z

signtool.exe sign /v /fd SHA256 /tr %TSERVER% /td sha256 7z1700-zstd-*.exe
cd %wd%

