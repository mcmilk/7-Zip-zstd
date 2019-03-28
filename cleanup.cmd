@echo off

ren Asm\arm arm_
FOR /R .\ %%d IN (arm x64 x86) DO rd /S /Q %%d 2>NUL
ren Asm\arm_ arm

del "CPP\7zip\*.7z" 2>NUL
del "CPP\7zip\*.exe" 2>NUL
rd /S /Q "CPP\7zip\bin*" 2>NUL
rd /S /Q "CPP\7zip\codecs" 2>NUL
rd /S /Q "CPP\7zip\totalcmd" 2>NUL

