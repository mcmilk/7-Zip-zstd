# Microsoft Developer Studio Project File - Name="Alone" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Alone - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Alone.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Alone.mak" CFG="Alone - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Alone - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Alone - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "FORMAT_BZIP2" /D "FORMAT_ZIP" /D "FORMAT_TAR" /D "FORMAT_GZIP" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE" /D "COMPRESS_IMPLODE" /D "COMPRESS_BZIP2" /D "CRYPTO_ZIP" /Yu"StdAfx.h" /FD /c
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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "FORMAT_BZIP2" /D "FORMAT_ZIP" /D "FORMAT_TAR" /D "FORMAT_GZIP" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE" /D "COMPRESS_IMPLODE" /D "COMPRESS_BZIP2" /D "CRYPTO_ZIP" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"c:\UTIL\7za.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Alone - Win32 Release"
# Name "Alone - Win32 Debug"
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
# Begin Group "SDK"

# PROP Default_Filter ""
# Begin Group "Compression"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\AriBitCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\AriBitCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\AriPrice.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\CopyCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\CopyCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\HuffmanDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\HuffmanEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\HuffmanEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\RangeCoder.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\AlignedBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\AlignedBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\CommandLineParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\CommandLineParser.h
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

SOURCE=..\..\..\..\SDK\Common\Exception.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\IntToString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\ListFileUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\ListFileUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\NewHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\Random.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StdInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StdInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StdOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StdOutStream.h
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

SOURCE=..\..\..\..\SDK\Common\Types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\UTFConvert.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\UTFConvert.h
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

SOURCE=..\..\..\..\SDK\Windows\Error.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Error.h
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

SOURCE=..\..\..\..\SDK\Windows\NationalTime.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\NationalTime.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\PropVariant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\PropVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\PropVariantConversions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\PropVariantConversions.h
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
# Begin Group "Archive"

# PROP Default_Filter ""
# Begin Group "Zip header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\Header.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\Header.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\Header.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\Header.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\InEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\InEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\InEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\InEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\ItemInfo.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\ItemInfo.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\ItemInfo.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\ItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\ItemInfoEx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\ItemNameUtils.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\ItemNameUtils.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\ItemNameUtils.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\ItemNameUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\OutEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\OutEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\OutEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Zip\OutEngine.h
# End Source File
# End Group
# Begin Group "Tar header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\Header.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Tar\Header.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Tar\Header.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\Header.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\InEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Tar\InEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Tar\InEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\InEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\ItemInfo.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Tar\ItemInfo.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Tar\ItemInfo.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\ItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\ItemInfoEx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\ItemNameUtils.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Tar\ItemNameUtils.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Tar\ItemNameUtils.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\ItemNameUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\OutEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Tar\OutEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Tar\OutEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Tar\OutEngine.h
# End Source File
# End Group
# Begin Group "GZip Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\Header.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\GZip\Header.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\GZip\Header.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\Header.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\InEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\GZip\InEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\GZip\InEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\InEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\ItemInfo.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\GZip\ItemInfo.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\GZip\ItemInfo.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\ItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\ItemInfoEx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\OutEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\GZip\OutEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\GZip\OutEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\GZip\OutEngine.h
# End Source File
# End Group
# End Group
# Begin Group "Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\FileStreams.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\FileStreams.h
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

SOURCE=..\..\..\..\SDK\Interface\OffsetStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Interface\OffsetStream.h
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

SOURCE=..\..\..\..\SDK\Stream\LSBFDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\LSBFDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\LSBFEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\LSBFEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\MSBFDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\MSBFEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\OutByte.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\OutByte.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\WindowIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\WindowIn.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\WindowOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\WindowOut.h
# End Source File
# End Group
# Begin Group "Util"

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
# Begin Group "Alien"

# PROP Default_Filter ""
# Begin Group "BZip2 Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\blocksort.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\bzlib.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\bzlib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\bzlib_private.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\compress.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\crctable.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\decompress.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\huffman.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Alien\Compress\BWT\BZip2\randtable.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# End Group
# End Group
# Begin Group "Format"

# PROP Default_Filter ""
# Begin Group "7z"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Format\7z\CompressionMethod.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Decode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Decode.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Encode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Encode.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Extract.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\FolderInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\FolderInStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\FolderOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\FolderOutStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Handler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Handler.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Header.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Header.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\InEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\InEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\ItemInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\ItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\ItemInfoUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\ItemInfoUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\ItemNameUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\ItemNameUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\MethodInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\MethodInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\OutEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\OutEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\OutHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\RegistryInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\UpdateEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\UpdateEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\UpdateItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\UpdateMain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\UpdateMain.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\UpdateSolidEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\UpdateSolidEngine.h
# End Source File
# End Group
# Begin Group "Format Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Format\Common\CoderMixer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\CoderMixer.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\CoderMixer2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\CoderMixer2.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\CrossThreadProgress.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\CrossThreadProgress.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\DummyOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\DummyOutStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\FormatCryptoInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\IArchiveHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\InStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\InStreamWithCRC.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\OutStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\OutStreamWithCRC.h
# End Source File
# End Group
# Begin Group "Zip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Format\Zip\AddCommon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\AddCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\CompressionMethod.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\Handler.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\Handler.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\Handler.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\Handler.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\OutHandler.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\OutHandler.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\OutHandler.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\UpdateEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\UpdateEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\UpdateEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\UpdateEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\UpdateItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\UpdateMain.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Zip\UpdateMain.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Zip\UpdateMain.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\Zip\UpdateMain.h
# End Source File
# End Group
# Begin Group "Tar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Format\Tar\Handler.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Tar\Handler.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Tar\Handler.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\Tar\Handler.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\Tar\OutHandler.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Tar\OutHandler.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Tar\OutHandler.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\Tar\UpdateEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Tar\UpdateEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Tar\UpdateEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\Tar\UpdateEngine.h
# End Source File
# End Group
# Begin Group "GZip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Format\GZip\AddCommon.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\GZip\AddCommon.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\GZip\AddCommon.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\GZip\AddCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\GZip\CompressionMethod.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\GZip\Handler.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\GZip\Handler.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\GZip\Handler.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\GZip\Handler.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\GZip\OutHandler.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\GZip\OutHandler.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\GZip\OutHandler.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\GZip\UpdateEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\GZip\UpdateEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\GZip\UpdateEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\GZip\UpdateEngine.h
# End Source File
# End Group
# Begin Group "BZip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Format\BZip2\Handler.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\BZip2\Handler.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\BZip2\Handler.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\BZip2\Handler.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\BZip2\ItemInfoEx.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\BZip2\OutHandler.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\BZip2\OutHandler.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\BZip2\OutHandler.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\BZip2\UpdateEngine.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\BZip2\UpdateEngine.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\BZip2\UpdateEngine.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Format\BZip2\UpdateEngine.h
# End Source File
# End Group
# End Group
# Begin Group "Compress"

# PROP Default_Filter ""
# Begin Group "Convert"

# PROP Default_Filter ""
# Begin Group "Branch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\Coder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\Coder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\x86.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\x86.h
# End Source File
# End Group
# End Group
# Begin Group "LZ"

# PROP Default_Filter ""
# Begin Group "LZMA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\AriConst.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\BitTreeCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\CoderInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\CoderInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\Decoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\Encoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\Encoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\LenCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\LenCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\LiteralCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\LiteralCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\LZMA.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\LZMA.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\LZMA\RCDefs.h
# End Source File
# End Group
# Begin Group "MatchFinder"

# PROP Default_Filter ""
# Begin Group "Pat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\Patricia\Pat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\Patricia\Pat2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\Patricia\Pat2H.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\Patricia\Pat2R.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\Patricia\PatMain.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\Patricia\Patricia.h
# End Source File
# End Group
# Begin Group "BinTree"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTree2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTree234.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTree3.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTree3Main.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTreeMain.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTreeMF.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTreeMFMain.h
# End Source File
# End Group
# End Group
# Begin Group "Deflate"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Deflate\Const.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Deflate\Decoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Deflate\Decoder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Deflate\Decoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Deflate\Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Deflate\Encoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Deflate\Encoder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Deflate\Encoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Deflate\Encoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Deflate\ExtConst.h
# End Source File
# End Group
# Begin Group "Implode"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Implode\Decoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Implode\Decoder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Implode\Decoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Implode\Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Implode\HuffmanDecoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\Implode\HuffmanDecoder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\Implode\HuffmanDecoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\Implode\HuffmanDecoder.h
# End Source File
# End Group
# End Group
# Begin Group "PPMD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\AriConst.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\Context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\Decode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\Decoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\PPMD\Decoder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\PPMD\Decoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\Encode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\Encoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\PPMD\Encoder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\PPMD\Encoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\Encoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\PPMdType.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\SubAlloc.h
# End Source File
# End Group
# Begin Group "BZip2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\BWT\BZip2\Decoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\BZip2\Decoder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\BZip2\Decoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\BWT\BZip2\Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\BWT\BZip2\Encoder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\BZip2\Encoder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\BZip2\Encoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\BWT\BZip2\Encoder.h
# End Source File
# End Group
# Begin Group "Compress Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\Interface\CompressInterface.h
# End Source File
# End Group
# End Group
# Begin Group "Console"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Console\AddSTD.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\AddSTD.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\ArError.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\CompressEngine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\CompressEngine.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\CompressionMethodUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\ConsoleCloseUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\ConsoleCloseUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\ExtractCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\ExtractCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\ExtractSTD.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\ExtractSTD.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\FileCreationUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\FileCreationUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\ListArchive.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\ListArchive.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\Main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\MainAr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\PercentPrinter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\PercentPrinter.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\UpdateArchiveOptions.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\UpdateCallback.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\UpdateCallback.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\UserInputUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\UserInputUtils.h
# End Source File
# End Group
# Begin Group "Archiver Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Common\ArchiveStyleDirItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\CompressEngineCommon.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\CompressEngineCommon.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\DefaultName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\DefaultName.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\IArchiveHandler2.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\OpenEngine200.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\OpenEngine200.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\PropIDUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\PropIDUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\SortUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\SortUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\UpdatePairBasic.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\UpdatePairInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\UpdatePairInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\UpdateProducer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\UpdateProducer.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\UpdateUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\UpdateUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\ZipRegistryMain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\ZipRegistryMain.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\ZipSettings.h
# End Source File
# End Group
# Begin Group "Crypto"

# PROP Default_Filter ""
# Begin Group "Cipher"

# PROP Default_Filter ""
# Begin Group "Zip Crypto"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\Zip\Coder.cpp

!IF  "$(CFG)" == "Alone - Win32 Release"

# ADD CPP /Fo"Release\ZipCrypto\Coder.obj"

!ELSEIF  "$(CFG)" == "Alone - Win32 Debug"

# ADD CPP /Fo"Debug\ZipCrypto\Coder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\Zip\Coder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\Zip\Crypto.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\Zip\Crypto.h
# End Source File
# End Group
# Begin Group "Cipher Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\Common\CipherInterface.h
# End Source File
# End Group
# End Group
# End Group
# End Target
# End Project
