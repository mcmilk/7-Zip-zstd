@echo off

FOR /R .\ %%d IN (AMD64 O) DO rd /S /Q %%d 2>NUL

del "CPP\7zip\*.7z" 2>NUL
del "CPP\7zip\7z*.exe" 2>NUL
rd /S /Q "CPP\7zip\bin32" 2>NUL
rd /S /Q "CPP\7zip\bin64" 2>NUL
rd /S /Q "CPP\7zip\codecs" 2>NUL
rd /S /Q "CPP\7zip\x32" 2>NUL
rd /S /Q "CPP\7zip\x64" 2>NUL

