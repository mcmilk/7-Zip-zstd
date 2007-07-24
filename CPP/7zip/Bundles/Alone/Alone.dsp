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
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /I "..\..\..\\" /D "NDEBUG" /D "_MBCS" /D "WIN32" /D "_CONSOLE" /D "COMPRESS_MF_MT" /D "COMPRESS_MT" /D "COMPRESS_BZIP2_MT" /D "BREAK_HANDLER" /D "_7ZIP_LARGE_PAGES" /D "BENCH_MT" /Yu"StdAfx.h" /FD /c
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
# ADD CPP /nologo /Gz /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\\" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_CONSOLE" /D "COMPRESS_MF_MT" /D "COMPRESS_MT" /D "COMPRESS_BZIP2_MT" /D "BREAK_HANDLER" /D "_7ZIP_LARGE_PAGES" /D "BENCH_MT" /Yu"StdAfx.h" /FD /GZ /c
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
# ADD CPP /nologo /Gz /MD /W3 /GX /O1 /I "..\..\..\\" /D "NDEBUG" /D "UNICODE" /D "_UNICODE" /D "WIN32" /D "_CONSOLE" /D "COMPRESS_MF_MT" /D "COMPRESS_MT" /D "COMPRESS_BZIP2_MT" /D "BREAK_HANDLER" /D "_7ZIP_LARGE_PAGES" /D "BENCH_MT" /Yu"StdAfx.h" /FD /c
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
# ADD CPP /nologo /Gz /W3 /Gm /GX /ZI /Od /I "..\..\..\\" /D "_DEBUG" /D "_UNICODE" /D "UNICODE" /D "WIN32" /D "_CONSOLE" /D "COMPRESS_MF_MT" /D "COMPRESS_MT" /D "COMPRESS_BZIP2_MT" /D "BREAK_HANDLER" /D "_7ZIP_LARGE_PAGES" /D "BENCH_MT" /Yu"StdAfx.h" /FD /GZ /c
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

SOURCE=..\..\UI\Console\ExtractCallbackConsole.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\ExtractCallbackConsole.h
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

SOURCE=..\..\UI\Console\OpenCallbackConsole.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\OpenCallbackConsole.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\PercentPrinter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\PercentPrinter.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\UpdateCallbackConsole.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Console\UpdateCallbackConsole.h
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

SOURCE=..\..\..\Common\AutoPtr.h
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

SOURCE=..\..\..\Common\Defs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\DynamicBuffer.h
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

SOURCE=..\..\..\Common\MyException.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyGuidDef.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyInitGuid.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyUnknown.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyVector.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyVector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyWindows.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyWindows.h
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

SOURCE=..\..\..\Windows\MemoryLock.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\MemoryLock.h
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

SOURCE=..\..\Common\CreateCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\CreateCoder.h
# End Source File
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

SOURCE=..\..\Common\FilterCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\FilterCoder.h
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

SOURCE=..\..\Common\LockedStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\LockedStream.h
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

SOURCE=..\..\Common\MemBlocks.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\MethodId.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\MethodId.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\MethodProps.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\MethodProps.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\MSBFDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\MSBFEncoder.h
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

SOURCE=..\..\Common\OutMemStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\OutMemStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\ProgressMt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\ProgressMt.h
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
# Begin Source File

SOURCE=..\..\Common\StreamUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\StreamUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\VirtThread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\VirtThread.h
# End Source File
# End Group
# Begin Group "Compress"

# PROP Default_Filter ""
# Begin Group "Branch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Branch\ARM.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\ARM.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\ARMThumb.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\ARMThumb.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BCJ2Register.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BCJRegister.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchRegister.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\Coder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\IA64.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\IA64.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\PPC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\PPC.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\SPARC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\SPARC.h
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
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2CRC.h
# End Source File
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

SOURCE=..\..\Compress\BZip2\BZip2Register.cpp
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
# Begin Source File

SOURCE=..\..\Compress\Copy\CopyRegister.cpp
# End Source File
# End Group
# Begin Group "Deflate"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Deflate\Deflate64Register.cpp
# End Source File
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
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateRegister.cpp
# End Source File
# End Group
# Begin Group "Huffman"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Huffman\HuffmanDecoder.h
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
# End Group
# Begin Source File

SOURCE=..\..\Compress\LZ\IMatchFinder.h
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

SOURCE=..\..\Compress\LZMA\LZMARegister.cpp
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

SOURCE=..\..\Compress\PPMD\PPMDRegister.cpp
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
# Begin Group "Shrink"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Shrink\ShrinkDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Shrink\ShrinkDecoder.h
# End Source File
# End Group
# Begin Group "Z"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Z\ZDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Z\ZDecoder.h
# End Source File
# End Group
# Begin Group "BWT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\BWT\BlockSort.cpp

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

SOURCE=..\..\Compress\BWT\BlockSort.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BWT\Mtf8.h
# End Source File
# End Group
# Begin Group "LZX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Lzx\Lzx.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Lzx\Lzx86Converter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Lzx\Lzx86Converter.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Lzx\LzxDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Lzx\LzxDecoder.h
# End Source File
# End Group
# Begin Group "Quantum"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Quantum\QuantumDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Quantum\QuantumDecoder.h
# End Source File
# End Group
# Begin Group "LZMA_Alone"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\LZMA_Alone\LzmaBench.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA_Alone\LzmaBench.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA_Alone\LzmaBenchCon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA_Alone\LzmaBenchCon.h
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

SOURCE=..\..\Archive\7z\7zRegister.cpp
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

SOURCE=..\..\Archive\BZip2\bz2Register.cpp
# End Source File
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

SOURCE=..\..\Archive\GZip\GZipRegister.cpp
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

SOURCE=..\..\Archive\Tar\TarRegister.cpp
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

SOURCE=..\..\Archive\Zip\ZipRegister.cpp
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

SOURCE=..\..\Archive\Common\CoderLoader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CoderMixer2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CoderMixer2.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CoderMixer2MT.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CoderMixer2MT.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\DummyOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\DummyOutStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\HandlerOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\HandlerOut.h
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

SOURCE=..\..\Archive\Common\MultiStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\MultiStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\OutStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\OutStreamWithCRC.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\ParseProperties.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\ParseProperties.h
# End Source File
# End Group
# Begin Group "split"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Split\SplitHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Split\SplitHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Split\SplitRegister.cpp
# End Source File
# End Group
# Begin Group "Z Format"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Z\ZHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Z\ZHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Z\ZRegister.cpp
# End Source File
# End Group
# Begin Group "cab"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Cab\CabBlockInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabBlockInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\CabRegister.cpp
# End Source File
# End Group
# End Group
# Begin Group "UI Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\UI\Common\ArchiveCommandLine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ArchiveCommandLine.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ArchiveExtractCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ArchiveExtractCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ArchiveOpenCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ArchiveOpenCallback.h
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

SOURCE=..\..\UI\Common\Extract.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\Extract.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ExtractingFilePath.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\ExtractingFilePath.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\LoadCodecs.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\LoadCodecs.h
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

SOURCE=..\..\UI\Common\SetProperties.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\SetProperties.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\SortUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\SortUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\TempFiles.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\TempFiles.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\Update.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\Update.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdateAction.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdateAction.h
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdateCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\UI\Common\UpdateCallback.h
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

SOURCE=..\..\Crypto\7zAES\7zAESRegister.cpp
# End Source File
# End Group
# Begin Group "WzAES"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\WzAES\WzAES.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\WzAES\WzAES.h
# End Source File
# End Group
# Begin Group "Hash"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\Hash\HmacSha1.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\HmacSha1.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Pbkdf2HmacSha1.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Pbkdf2HmacSha1.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\RandGen.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\RandGen.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Sha1.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Sha1.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Sha256.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Sha256.h
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
# Begin Group "C"

# PROP Default_Filter ""
# Begin Group "C-Compress"

# PROP Default_Filter ""
# Begin Group "C Lz"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\LzHash.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\MatchFinder.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\MatchFinderMt.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\MatchFinderMt.h
# End Source File
# End Group
# Begin Group "C Huffman"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Huffman\HuffmanEncode.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Huffman\HuffmanEncode.h
# End Source File
# End Group
# Begin Group "C Branch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARM.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARMThumb.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARMThumb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchIA64.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchIA64.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchPPC.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchPPC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchSPARC.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchSPARC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchX86.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchX86.h
# End Source File
# End Group
# End Group
# Begin Group "C-Crypto"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\C\Crypto\Aes.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Crypto\Aes.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\..\C\7zCrc.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\7zCrc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Alloc.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Alloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\IStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Sort.c

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 ReleaseU"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "Alone - Win32 DebugU"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Sort.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Threads.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Threads.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Types.h
# End Source File
# End Group
# End Target
# End Project
