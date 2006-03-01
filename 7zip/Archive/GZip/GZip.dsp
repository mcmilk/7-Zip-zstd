# Microsoft Developer Studio Project File - Name="GZip" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=GZip - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GZip.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GZip.mak" CFG="GZip - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GZip - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "GZip - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GZip - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GZIP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /I "..\..\..\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GZIP_EXPORTS" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-Zip\Formats\gz.dll" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "GZip - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GZIP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GZIP_EXPORTS" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-Zip\Formats\gz.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "GZip - Win32 Release"
# Name "GZip - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Archive.def
# End Source File
# Begin Source File

SOURCE=.\DllExports.cpp
# End Source File
# Begin Source File

SOURCE=.\GZip.ico
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"StdAfx.h"
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Common\Alloc.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\CRC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\NewHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\String.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\String.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\StringConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\StringConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\StringToInt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\StringToInt.h
# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Windows\DLL.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\DLL.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\PropVariant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\PropVariant.h
# End Source File
# End Group
# Begin Group "Compression"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Copy\CopyCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Copy\CopyCoder.h
# End Source File
# End Group
# Begin Group "Engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\GZipHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\GZipHandler.h
# End Source File
# Begin Source File

SOURCE=.\GZipHandlerOut.cpp
# End Source File
# Begin Source File

SOURCE=.\GZipHeader.cpp
# End Source File
# Begin Source File

SOURCE=.\GZipHeader.h
# End Source File
# Begin Source File

SOURCE=.\GZipIn.cpp
# End Source File
# Begin Source File

SOURCE=.\GZipIn.h
# End Source File
# Begin Source File

SOURCE=.\GZipItem.h
# End Source File
# Begin Source File

SOURCE=.\GZipOut.cpp
# End Source File
# Begin Source File

SOURCE=.\GZipOut.h
# End Source File
# Begin Source File

SOURCE=.\GZipUpdate.cpp
# End Source File
# Begin Source File

SOURCE=.\GZipUpdate.h
# End Source File
# End Group
# Begin Group "7zip Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Common\ProgressUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\ProgressUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\StreamUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\StreamUtils.h
# End Source File
# End Group
# Begin Group "Archive Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\CodecsPath.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\CodecsPath.h
# End Source File
# Begin Source File

SOURCE=..\Common\CoderLoader.h
# End Source File
# Begin Source File

SOURCE=..\Common\InStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\InStreamWithCRC.h
# End Source File
# Begin Source File

SOURCE=..\Common\OutStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\OutStreamWithCRC.h
# End Source File
# Begin Source File

SOURCE=..\Common\ParseProperties.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\ParseProperties.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\gz.ico
# End Source File
# End Target
# End Project
