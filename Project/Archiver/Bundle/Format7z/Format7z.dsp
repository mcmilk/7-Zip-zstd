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
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\..\SDK" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_MF_HC" /D "COMPRESS_PPMD" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"C:\Program Files\7-ZIP\Format\7za.dll" /opt:NOWIN98
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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\SDK" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MY7Z_EXPORTS" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_BCJ2" /D "COMPRESS_COPY" /D "COMPRESS_MF_PAT" /D "COMPRESS_MF_BT" /D "COMPRESS_MF_HC" /D "COMPRESS_PPMD" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"C:\Program Files\7-ZIP\Format\7za.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "7z - Win32 Release"
# Name "7z - Win32 Debug"
# Begin Group "Spec"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Format\7z\7z.def
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\DllExports.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\resource.rc
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\StdAfx.cpp
# ADD CPP /Yc
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\StdAfx.h
# End Source File
# End Group
# Begin Group "Header"

# PROP Default_Filter ""
# Begin Group "Common Header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Common\ItemNameUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Common\ItemNameUtils.h
# End Source File
# End Group
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

SOURCE=..\..\..\..\SDK\Windows\Synchronization.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\Synchronization.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\System.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Windows\System.h
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

SOURCE=..\..\..\..\SDK\Compression\AriBitCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\AriBitCoder.h
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
# Begin Group "Engine"

# PROP Default_Filter ""
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

SOURCE=..\..\Format\7z\Handler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\Handler.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\ItemInfoUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\ItemInfoUtils.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\OutHandler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\StreamObjects2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\StreamObjects2.h
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
# Begin Group "Format common"

# PROP Default_Filter ""
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
# Begin Group "Compress"

# PROP Default_Filter ""
# Begin Group "Convert"

# PROP Default_Filter ""
# Begin Group "Branch"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\Coder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\x86.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\x86.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\x86_2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\Convert\Branch\x86_2.h
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

SOURCE=..\..\..\Compress\LZ\MatchFinder\Patricia\Pat3H.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\Patricia\Pat4H.h
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

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTree3.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\BinTree\BinTree4.h
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
# Begin Group "MT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\MT\MT.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\MT\MT.h
# End Source File
# End Group
# Begin Group "HashChain"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\HashChain\HC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\HashChain\HC3.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\HashChain\HC4.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\HashChain\HCMain.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\HashChain\HCMF.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\LZ\MatchFinder\HashChain\HCMFMain.h
# End Source File
# End Group
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

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /Fo"Release\PPMD\Decoder.obj"

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

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

!IF  "$(CFG)" == "7z - Win32 Release"

# ADD CPP /Fo"Release\PPMD\Encoder.obj"

!ELSEIF  "$(CFG)" == "7z - Win32 Debug"

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
# Begin Group "Compress Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Compress\Interface\CompressInterface.h
# End Source File
# End Group
# End Group
# Begin Group "Crypto"

# PROP Default_Filter ""
# Begin Group "Cipher"

# PROP Default_Filter ""
# Begin Group "AES"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\aes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\AES_CBC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\aescpp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\aescrypt.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\aeskey.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\aesopt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\aestab.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\MyAES.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\AES\MyAES.h
# End Source File
# End Group
# Begin Group "7zAES"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\7zAES\7zAES.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\7zAES\7zAES.h
# End Source File
# End Group
# End Group
# Begin Group "Hash"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Crypto\Hash\SHA256\SHA256.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Hash\SHA256\SHA256.h
# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=".\7-7z.ico"
# End Source File
# Begin Source File

SOURCE="..\..\Format\7z\7-7z.ico"
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\CompressionMethod.h
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
# End Target
# End Project
