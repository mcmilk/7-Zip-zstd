@echo off

FOR /R .\ %%d IN (arm x64 O) DO rd /S /Q %%d 2>NUL

del "CPP\7zip\*.7z" 2>NUL
del "CPP\7zip\*.exe" 2>NUL
rd /S /Q "CPP\7zip\bin*" 2>NUL
rd /S /Q "CPP\7zip\codecs" 2>NUL
rd /S /Q "CPP\7zip\totalcmd" 2>NUL

