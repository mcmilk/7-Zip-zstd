# Microsoft Developer Studio Project File - Name="7z" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=7z - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Format7z.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Format7z.mak" CFG="7z - Win32 Debug"
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
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /I "..\..\..\\" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_MT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE_DECODER" /D "COMPRESS_BZIP2_DECODER" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-Zip\Formats\7za.dll" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none /debug

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
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\SDK" /I "..\..\..\\" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_MT" /D "COMPRESS_PPMD" /D "COMPRESS_DEFLATE_DECODER" /D "COMPRESS_BZIP2_DECODER" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-Zip\Formats\7za.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "7z - Win32 Release"
# Name "7z - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\7z\7z.def
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\7z.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\7z\DllExports.cpp
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

SOURCE=..\..\..\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\IntToString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\NewHandler.cpp
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

SOURCE=..\..\..\Windows\PropVariant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\PropVariant.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Synchronization.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Synchronization.h
# End Source File
# End Group
# Begin Group "Archive common"

# PROP Default_Filter ""
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

SOURCE=..\..\Archive\Common\CrossThreadProgress.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\CrossThreadProgress.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\FilterCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\FilterCoder.h
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
# Begin Group "Compress"

# PROP Default_Filter ""
# Begin Group "LZ"

# PROP Default_Filter ""
# Begin Group "MT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\LZ\MT\MT.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\MT\MT.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Compress\LZ\LZInWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\LZInWindow.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\LZOutWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZ\LZOutWindow.h
# End Source File
# End Group
# Begin Group "PPMD"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDContext.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDDecode.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\PPMD\PPMDDecoder.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

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

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

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
# Begin Group "Branch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\BranchCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\x86.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\x86.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\x86_2.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\Branch\x86_2.h
# End Source File
# End Group
# Begin Group "LZMA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMA.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMADecoder.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMADecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMAEncoder.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMAEncoder.h
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
# Begin Group "RangeCoder"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\RangeCoder\RangeCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\RangeCoder\RangeCoderBit.cpp
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
# Begin Group "Deflate"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateConst.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateDecoder.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateDecoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateExtConst.h
# End Source File
# End Group
# Begin Group "BZip2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Const.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2CRC.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Decoder.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Decoder.h
# End Source File
# End Group
# End Group
# Begin Group "Crypto"

# PROP Default_Filter ""
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

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\aeskey.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\aesopt.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\aestab.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\MyAES.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\AES\MyAES.h
# End Source File
# End Group
# Begin Group "7zAES"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\7zAES\7zAES.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

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

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\7zAES\SHA256.h
# End Source File
# End Group
# End Group
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

SOURCE=..\..\Archive\7z\7zMethods.h
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
# Begin Source File

SOURCE=..\..\Archive\7z\7zUpdateItem.h
# End Source File
# End Group
# Begin Group "7zip Common"

# PROP Default_Filter ""
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
# End Target
# End Project
