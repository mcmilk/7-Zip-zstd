# Microsoft Developer Studio Project File - Name="Alone" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Alone - Win32 DebugU
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Alone.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Alone.mak" CFG="Alone - Win32 DebugU"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Alone - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Alone - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "Alone - Win32 ReleaseU" (based on "Win32 (x86) Console Application")
!MESSAGE "Alone - Win32 DebugU" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Alone - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /I "..\..\..\\" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_CONSOLE" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "FORMAT_BZIP2" /D "FORMAT_ZIP" /D "FORMAT_TAR" /D "FORMAT_GZIP" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_MF_HC" /D "COMPRESS_MF_MT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE" /D "COMPRESS_DEFLATE64" /D "COMPRESS_IMPLODE" /D "COMPRESS_BZIP2" /D "CRYPTO_ZIP" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"c:\UTIL\7za.exe" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /Gz /W3 /Gm /GX /ZI /Od /I "..\..\..\\" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_CONSOLE" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "FORMAT_BZIP2" /D "FORMAT_ZIP" /D "FORMAT_TAR" /D "FORMAT_GZIP" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_MF_HC" /D "COMPRESS_MF_MT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE" /D "COMPRESS_DEFLATE64" /D "COMPRESS_IMPLODE" /D "COMPRESS_BZIP2" /D "CRYPTO_ZIP" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"c:\UTIL\7za.exe" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseU"
# PROP BASE Intermediate_Dir "ReleaseU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseU"
# PROP Intermediate_Dir "ReleaseU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "FORMAT_BZIP2" /D "FORMAT_ZIP" /D "FORMAT_TAR" /D "FORMAT_GZIP" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE" /D "COMPRESS_IMPLODE" /D "COMPRESS_BZIP2" /D "CRYPTO_ZIP" /Yu"StdAfx.h" /FD /c
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /I "..\..\..\\" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_CONSOLE" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "FORMAT_BZIP2" /D "FORMAT_ZIP" /D "FORMAT_TAR" /D "FORMAT_GZIP" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_MF_HC" /D "COMPRESS_MF_MT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE" /D "COMPRESS_DEFLATE64" /D "COMPRESS_IMPLODE" /D "COMPRESS_BZIP2" /D "CRYPTO_ZIP" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"c:\UTIL\7za.exe" /opt:NOWIN98
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"c:\UTIL\7zan.exe" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugU"
# PROP BASE Intermediate_Dir "DebugU"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugU"
# PROP Intermediate_Dir "DebugU"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "FORMAT_BZIP2" /D "FORMAT_ZIP" /D "FORMAT_TAR" /D "FORMAT_GZIP" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE" /D "COMPRESS_IMPLODE" /D "COMPRESS_BZIP2" /D "CRYPTO_ZIP" /D "_MBCS" /Yu"StdAfx.h" /FD /GZ /c
# ADD CPP /nologo /Gz /W3 /Gm /GX /ZI /Od /I "..\..\..\\" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_CONSOLE" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "FORMAT_BZIP2" /D "FORMAT_ZIP" /D "FORMAT_TAR" /D "FORMAT_GZIP" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_MF_HC" /D "COMPRESS_MF_MT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE" /D "COMPRESS_DEFLATE64" /D "COMPRESS_IMPLODE" /D "COMPRESS_BZIP2" /D "CRYPTO_ZIP" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"c:\UTIL\7za.exe" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"c:\UTIL\7zan.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Alone - Win32 Release"
# Name "Alone - Win32 Debug"
# Name "Alone - Win32 ReleaseU"
# Name "Alone - Win32 DebugU"
# Begin Group "Console"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\UI\Console\ArError.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\CompressionMode.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\ConsoleClose.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\ConsoleClose.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\Extract.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\Extract.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\ExtractCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\ExtractCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\List.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\List.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\Main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\MainAr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\OpenCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\OpenCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\PercentPrinter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\PercentPrinter.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\TempFiles.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\TempFiles.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\Update.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\Update.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\UpdateCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\UpdateCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\UserInputUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\UserInputUtils.h
# End Source File
# End Group
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\resource.h
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

SOURCE=..\..\..\Common\AlignedBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\AlignedBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\CommandLineParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\CommandLineParser.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\ComTry.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\CRC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\DynamicBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Exception.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\IntToString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\ListFileUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\ListFileUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyCom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\NewHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Random.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\StdInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\StdInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\StdOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\StdOutStream.h
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
# Begin Source File

SOURCE=..\..\..\Common\Types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\UTFConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\UTFConvert.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Vector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Wildcard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\Wildcard.h
# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Windows\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Device.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\DLL.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\DLL.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Error.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Error.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\FileDir.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\FileDir.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\FileFind.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\FileFind.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\FileIO.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\FileIO.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\FileName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\FileName.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Handle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\PropVariant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\PropVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\PropVariantConversions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\PropVariantConversions.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Synchronization.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Synchronization.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\System.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\System.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Thread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Time.h
# End Source File
# End Group
# Begin Group "7zip Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Common\CrossThreadProgress.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CrossThreadProgress.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\FilePathAutoRename.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\FilePathAutoRename.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\FileStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\FileStreams.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\InBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\InBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\InOutTempBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\InOutTempBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\LimitedStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\LimitedStreams.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\LSBFDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\LSBFDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\LSBFEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\LSBFEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\MSBFDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\MSBFEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\MultiStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\MultiStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\OffsetStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\OffsetStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\OutBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\OutBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\ProgressUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\ProgressUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\StreamBinder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\StreamBinder.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\StreamObjects.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\StreamObjects.h
# End Source File
# End Group
# Begin Group "Compress"

# PROP Default_Filter ""
# Begin Group "Branch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Branch\Coder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\x86.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\x86.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\x86_2.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\x86_2.h
# End Source File
# End Group
# Begin Group "BZip2"

# PROP Default_Filter ""
# Begin Group "Original BZip2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\blocksort.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\bzlib.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\bzlib.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\bzlib_private.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\compress.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\crctable.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\decompress.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\huffman.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\Original\randtable.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Decoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Encoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Encoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Error.cpp
# End Source File
# End Group
# Begin Group "Copy"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Copy\CopyCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Copy\CopyCoder.h
# End Source File
# End Group
# Begin Group "Deflate"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateConst.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateDecoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateEncoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateExtConst.h
# End Source File
# End Group
# Begin Group "Huffman"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Huffman\HuffmanDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Huffman\HuffmanEncoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\Huffman\HuffmanEncoder.h
# End Source File
# End Group
# Begin Group "Implode"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Implode\ImplodeDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Implode\ImplodeDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Implode\ImplodeHuffmanDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Implode\ImplodeHuffmanDecoder.h
# End Source File
# End Group
# Begin Group "LZ"

# PROP Default_Filter ""
# Begin Group "MT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\LZ\MT\MT.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\MT\MT.h
# End Source File
# End Group
# Begin Group "HC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\LZ\HashChain\HC.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\HashChain\HC2.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\HashChain\HC3.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\HashChain\HC4.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\HashChain\HC4b.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\HashChain\HCMain.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\HashChain\HCMF.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\HashChain\HCMFMain.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Compress\LZ\IMatchFinder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\LZInWindow.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\LZInWindow.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\LZOutWindow.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\LZOutWindow.h
# End Source File
# End Group
# Begin Group "LZMA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMA.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMADecoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMADecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMAEncoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMAEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMALen.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMALen.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMALiteral.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMALiteral.h
# End Source File
# End Group
# Begin Group "PPMd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDContext.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDDecode.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDDecoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDEncode.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDEncoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDSubAlloc.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDType.h
# End Source File
# End Group
# Begin Group "RangeCoder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\RangeCoder\RangeCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\RangeCoder\RangeCoderBit.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\RangeCoder\RangeCoderBit.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\RangeCoder\RangeCoderBitTree.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\RangeCoder\RangeCoderOpt.h
# End Source File
# End Group
# End Group
# Begin Group "Archive"

# PROP Default_Filter ""
# Begin Group "7z"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\7z\7zCompressionMode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zCompressionMode.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zDecode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zDecode.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zEncode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zEncode.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zExtract.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zFolderInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zFolderInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zFolderOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zFolderOutStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zHandlerOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zMethodID.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zMethodID.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zOut.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zProperties.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zProperties.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zSpecStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zSpecStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zUpdate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7zUpdate.h
# End Source File
# End Group
# Begin Group "bz2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\BZip2\BZip2Handler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\BZip2\BZip2Handler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\BZip2\BZip2HandlerOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\BZip2\BZip2Item.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\BZip2\BZip2Update.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\BZip2\BZip2Update.h
# End Source File
# End Group
# Begin Group "gz"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipHandlerOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipOut.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipUpdate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\GZipUpdate.h
# End Source File
# End Group
# Begin Group "tar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Tar\TarHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarHandlerOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarOut.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarUpdate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\TarUpdate.h
# End Source File
# End Group
# Begin Group "zip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipAddCommon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipAddCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipCompressionMode.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipHandlerOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipItem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipItemEx.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipOut.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipUpdate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\ZipUpdate.h
# End Source File
# End Group
# Begin Group "Archive Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Common\CoderMixer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CoderMixer.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CoderMixer2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CoderMixer2.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\DummyOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\DummyOutStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\InStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\InStreamWithCRC.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\ItemNameUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\ItemNameUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\OutStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\OutStreamWithCRC.h
# End Source File
# End Group
# End Group
# Begin Group "UI Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\UI\Common\ArchiverInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ArchiverInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\DefaultName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\DefaultName.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\EnumDirItems.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\EnumDirItems.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ExtractingFilePath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ExtractingFilePath.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\OpenArchive.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\OpenArchive.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\PropIDUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\PropIDUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\SortUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\SortUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdateAction.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdateAction.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdatePair.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdatePair.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdateProduce.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdateProduce.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\WorkDir.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\WorkDir.h
# End Source File
# End Group
# Begin Group "Crypto"

# PROP Default_Filter ""
# Begin Group "Zip Crypto"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\Zip\ZipCipher.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Zip\ZipCipher.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Zip\ZipCrypto.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O1

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Zip\ZipCrypto.h
# End Source File
# End Group
# Begin Group "AES"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\AES\aes.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\AES_CBC.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\aescpp.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\aescrypt.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\aeskey.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\aesopt.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\aestab.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\MyAES.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\MyAES.h
# End Source File
# End Group
# Begin Group "7z AES"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\7zAES\7zAES.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\7zAES\7zAES.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\7zAES\MySHA256.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\7zAES\SHA256.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\7zAES\SHA256.h
# End Source File
# End Group
# End Group
# Begin Group "7-zip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ICoder.h
# End Source File
# Begin Source File

SOURCE=..\..\IMyUnknown.h
# End Source File
# Begin Source File

SOURCE=..\..\IPassword.h
# End Source File
# Begin Source File

SOURCE=..\..\IProgress.h
# End Source File
# Begin Source File

SOURCE=..\..\IStream.h
# End Source File
# Begin Source File

SOURCE=..\..\PropID.h
# End Source File
# End Group
# End Target
# End Project
