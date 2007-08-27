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
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /I "..\..\..\\" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /D "NO_REGISTRY" /D "COMPRESS_MF_MT" /D "COMPRESS_MT" /D "COMPRESS_BZIP2_MT" /D "EXTERNAL_CODECS" /D "_7ZIP_LARGE_PAGES" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-Zip\7z.dll" /opt:NOWIN98
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
# ADD CPP /nologo /Gz /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\SDK" /I "..\..\..\\" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /D "NO_REGISTRY" /D "COMPRESS_MF_MT" /D "COMPRESS_MT" /D "COMPRESS_BZIP2_MT" /D "EXTERNAL_CODECS" /D "_7ZIP_LARGE_PAGES" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-Zip\7z.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "7z - Win32 Release"
# Name "7z - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\7z\7z.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Archive2.def
# End Source File
# Begin Source File

SOURCE=..\..\Archive\ArchiveExports.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\CodecExports.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\DllExports2.cpp
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

SOURCE=..\..\..\Common\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\IntToString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\IntToString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyCom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\MyException.h
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

SOURCE=..\..\..\Common\MyWindows.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\NewHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Common\NewHandler.h
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
# Begin Source File

SOURCE=..\..\..\Windows\System.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\System.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Windows\Thread.h
# End Source File
# End Group
# Begin Group "Compress"

# PROP Default_Filter ""
# Begin Group "LZ"

# PROP Default_Filter ""
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

SOURCE=..\..\Compress\PPMD\PPMDRegister.cpp
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
# Begin Source File

SOURCE=..\..\Compress\LZMA\LZMARegister.cpp
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

SOURCE=..\..\Compress\Deflate\Deflate64Register.cpp
# End Source File
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

SOURCE=..\..\Compress\Deflate\DeflateEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateEncoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateNsisRegister.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Deflate\DeflateRegister.cpp
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
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Encoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Encoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BZip2\BZip2Register.cpp
# End Source File
# End Group
# Begin Group "Rar Codecs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Rar\Rar1Decoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Rar\Rar1Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Rar\Rar2Decoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Rar\Rar2Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Rar\Rar3Decoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Rar\Rar3Decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Rar\Rar3Vm.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Rar\Rar3Vm.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Rar\RarCodecsRegister.cpp
# End Source File
# End Group
# Begin Group "BWT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\BWT\BlockSort.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BWT\BlockSort.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\BWT\Mtf8.h
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
# Begin Group "Lzx"

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
# Begin Group "Z Codec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Z\ZDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Z\ZDecoder.h
# End Source File
# End Group
# Begin Group "Arj Codecs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Arj\ArjDecoder1.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Arj\ArjDecoder1.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Arj\ArjDecoder2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Arj\ArjDecoder2.h
# End Source File
# End Group
# Begin Group "ByteSwap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\ByteSwap\ByteSwap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\ByteSwap\ByteSwap.h
# End Source File
# Begin Source File

SOURCE=..\..\Compress\ByteSwap\ByteSwapRegister.cpp
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
# Begin Group "Quantum"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Quantum\QuantumDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Quantum\QuantumDecoder.h
# End Source File
# End Group
# Begin Group "Lzh Codecs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Compress\Lzh\LzhDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Compress\Lzh\LzhDecoder.h
# End Source File
# End Group
# End Group
# Begin Group "Crypto"

# PROP Default_Filter ""
# Begin Group "AES"

# PROP Default_Filter ""
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

SOURCE=..\..\Crypto\7zAES\7zAESRegister.cpp
# End Source File
# End Group
# Begin Group "Hash"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\Hash\HmacSha1.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\HmacSha1.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Pbkdf2HmacSha1.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Pbkdf2HmacSha1.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\RandGen.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\RandGen.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Sha1.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Sha1.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Sha256.cpp

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Hash\Sha256.h
# End Source File
# End Group
# Begin Group "RarAES"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\RarAES\RarAES.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\RarAES\RarAES.h
# End Source File
# End Group
# Begin Group "RarCrypto"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\Rar20\Rar20Cipher.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Rar20\Rar20Cipher.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Rar20\Rar20Crypto.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Rar20\Rar20Crypto.h
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
# Begin Group "ZipCrypto"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Crypto\Zip\ZipCipher.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Zip\ZipCipher.h
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Zip\ZipCrypto.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Crypto\Zip\ZipCrypto.h
# End Source File
# End Group
# Begin Group "ZipStrong"

# PROP Default_Filter ""
# End Group
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

SOURCE=..\..\Common\MemBlocks.h
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

SOURCE=..\..\Common\RegisterArc.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\RegisterCodec.h
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
# Begin Group "C"

# PROP Default_Filter ""
# Begin Group "Compress C"

# PROP Default_Filter ""
# Begin Group "C-Lz"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\LzHash.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\MatchFinder.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\MatchFinder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\MatchFinderMt.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Lz\MatchFinderMt.h
# End Source File
# End Group
# Begin Group "Huffman"

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

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARMThumb.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchARMThumb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchIA64.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchIA64.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchPPC.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchPPC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchSPARC.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchSPARC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchX86.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2 /FAs
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Compress\Branch\BranchX86.h
# End Source File
# End Group
# End Group
# Begin Group "Crypto-C"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\C\Crypto\Aes.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Crypto\Aes.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\..\C\7zCrc.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

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

SOURCE=..\..\..\..\C\CpuArch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\IStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\C\Sort.c

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /O2
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

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
# Begin Source File

SOURCE=..\..\Archive\7z\7zUpdateItem.h
# End Source File
# End Group
# Begin Group "Rar"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Rar\RarHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarItem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarRegister.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarVolumeInStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\RarVolumeInStream.h
# End Source File
# End Group
# Begin Group "Arj"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Arj\ArjHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Arj\ArjHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Arj\ArjHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Arj\ArjIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Arj\ArjIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Arj\ArjItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Arj\ArjRegister.cpp
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
# Begin Group "Cab"

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
# Begin Group "Chm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Chm\ChmHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Chm\ChmHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Chm\ChmHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Chm\ChmHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Chm\ChmIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Chm\ChmIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Chm\ChmRegister.cpp
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

SOURCE=..\..\Archive\Common\OutStreamWithSha1.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\OutStreamWithSha1.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\ParseProperties.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Common\ParseProperties.h
# End Source File
# End Group
# Begin Group "Cpio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Cpio\CpioHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cpio\CpioHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cpio\CpioHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cpio\CpioHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cpio\CpioIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cpio\CpioIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cpio\CpioItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cpio\CpioRegister.cpp
# End Source File
# End Group
# Begin Group "Deb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Deb\DebHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Deb\DebHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Deb\DebHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Deb\DebHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Deb\DebIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Deb\DebIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Deb\DebItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Deb\DebRegister.cpp
# End Source File
# End Group
# Begin Group "GZip"

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
# Begin Group "Iso"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Iso\IsoHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Iso\IsoHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Iso\IsoHeader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Iso\IsoHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Iso\IsoIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Iso\IsoIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Iso\IsoItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Iso\IsoRegister.cpp
# End Source File
# End Group
# Begin Group "Lzh"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhCRC.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhItem.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhOutStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhOutStreamWithCRC.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\LzhRegister.cpp
# End Source File
# End Group
# Begin Group "Nsis"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Nsis\NsisDecode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Nsis\NsisDecode.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Nsis\NsisHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Nsis\NsisHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Nsis\NsisIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Nsis\NsisIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Nsis\NsisRegister.cpp
# End Source File
# End Group
# Begin Group "RPM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\RPM\RpmHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\RPM\RpmHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\RPM\RpmHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\RPM\RpmIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\RPM\RpmIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\RPM\RpmRegister.cpp
# End Source File
# End Group
# Begin Group "Split"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Split\SplitHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Split\SplitHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Split\SplitHandlerOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Split\SplitRegister.cpp
# End Source File
# End Group
# Begin Group "Tar"

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
# Begin Group "Z"

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
# Begin Group "Zip"

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
# Begin Group "Wim"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Wim\WimHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Wim\WimHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Wim\WimIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Wim\WimIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Wim\WimRegister.cpp
# End Source File
# End Group
# Begin Group "Com"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Archive\Com\ComHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Com\ComHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Com\ComIn.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Com\ComIn.h
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Com\ComRegister.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\Archive\IArchive.h
# End Source File
# End Group
# Begin Group "7zip"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ICoder.h
# End Source File
# Begin Source File

SOURCE=..\..\IDecl.h
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
# End Group
# Begin Source File

SOURCE=..\..\Archive\Arj\arj.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\BZip2\bz2.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cab\cab.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Cpio\cpio.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Deb\deb.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\GZip\gz.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Iso\Iso.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Lzh\lzh.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Rar\rar.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\RPM\rpm.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Split\Split.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Tar\tar.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Wim\wim.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Z\Z.ico
# End Source File
# Begin Source File

SOURCE=..\..\Archive\Zip\zip.ico
# End Source File
# End Target
# End Project
