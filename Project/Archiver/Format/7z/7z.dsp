# Microsoft Developer Studio Project File - Name="7z" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=7z - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "7z.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "7z.mak" CFG="7z - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "7z - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "7z - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "7z - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-ZIP\Format\7z.dll" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-ZIP\Format\7z.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "7z - Win32 Release"
# Name "7z - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\7z.def
# End Source File
# Begin Source File

SOURCE=.\DllExports.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Header.cpp
# End Source File
# Begin Source File

SOURCE=.\Header.h
# End Source File
# Begin Source File

SOURCE=.\InEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\InEngine.h
# End Source File
# Begin Source File

SOURCE=.\ItemInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\ItemInfo.h
# End Source File
# Begin Source File

SOURCE=.\ItemNameUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\ItemNameUtils.h
# End Source File
# Begin Source File

SOURCE=.\MethodInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\MethodInfo.h
# End Source File
# Begin Source File

SOURCE=.\OutEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\OutEngine.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\CRC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\DynamicBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\NewHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StdInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StdInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\String.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StringConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StringConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Vector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Wildcard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Wildcard.h
# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\COM.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\COM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\COMTry.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\FileDir.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\FileDir.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\FileFind.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\FileFind.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\FileIO.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\FileIO.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\FileName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\FileName.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Handle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\PropVariant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\PropVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Registry.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Registry.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Synchronization.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Synchronization.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Thread.h
# End Source File
# End Group
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\Interface\CompressInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Hash\Common\CryptoHashInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\IArchiveHandler2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\ICoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\IInOutStreams.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\IProgress.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\LimitedStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\LimitedStreams.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\ProgressUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\ProgressUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\StreamObjects.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\StreamObjects.h
# End Source File
# End Group
# Begin Group "Stream"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\InByte.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\InByte.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\OutByte.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\OutByte.h
# End Source File
# End Group
# Begin Group "Utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Util\InOutTempBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Util\InOutTempBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Util\MultiStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Util\MultiStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Util\StreamBinder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Util\StreamBinder.h
# End Source File
# End Group
# Begin Group "Compression"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\CopyCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\CopyCoder.h
# End Source File
# End Group
# Begin Group "Engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Decode.cpp
# End Source File
# Begin Source File

SOURCE=.\Decode.h
# End Source File
# Begin Source File

SOURCE=.\Encode.cpp
# End Source File
# Begin Source File

SOURCE=.\Encode.h
# End Source File
# Begin Source File

SOURCE=.\Extract.cpp
# End Source File
# Begin Source File

SOURCE=.\Handler.cpp
# End Source File
# Begin Source File

SOURCE=.\Handler.h
# End Source File
# Begin Source File

SOURCE=.\ItemInfoUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\ItemInfoUtils.h
# End Source File
# Begin Source File

SOURCE=.\OutHandler.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateEngine.h
# End Source File
# Begin Source File

SOURCE=.\UpdateItemInfo.h
# End Source File
# Begin Source File

SOURCE=.\UpdateMain.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateMain.h
# End Source File
# Begin Source File

SOURCE=.\UpdateSolidEngine.cpp
# End Source File
# Begin Source File

SOURCE=.\UpdateSolidEngine.h
# End Source File
# End Group
# Begin Group "Format common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Common\CoderMixer2.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\CoderMixer2.h
# End Source File
# Begin Source File

SOURCE=..\Common\CrossThreadProgress.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\CrossThreadProgress.h
# End Source File
# Begin Source File

SOURCE=..\Common\IArchiveHandler.h
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
# End Group
# Begin Source File

SOURCE=".\7-7z.ico"
# End Source File
# Begin Source File

SOURCE=.\CompressionMethod.h
# End Source File
# Begin Source File

SOURCE=.\FolderInStream.cpp
# End Source File
# Begin Source File

SOURCE=.\FolderInStream.h
# End Source File
# Begin Source File

SOURCE=.\FolderOutStream.cpp
# End Source File
# Begin Source File

SOURCE=.\FolderOutStream.h
# End Source File
# Begin Source File

SOURCE=.\RegistryInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\RegistryInfo.h
# End Source File
# End Target
# End Project
