# Microsoft Developer Studio Project File - Name="SFXCon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=SFXCon - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SFXCon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SFXCon.mak" CFG="SFXCon - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SFXCon - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "SFXCon - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SFXCon - Win32 Release"

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
# ADD CPP /nologo /Gz /MT /W3 /GX /O1 /I "..\..\..\..\SDK" /D "NDEBUG" /D "COMPRESS_BCJ2" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EXTRACT_ONLY" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_COPY" /D "COMPRESS_PPMD" /D "_SFX" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"C:\Util\7zSfxCon.exe" /opt:NOWIN98
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "SFXCon - Win32 Debug"

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
# ADD CPP /nologo /Gz /W3 /Gm /GX /ZI /Od /I "..\..\..\..\SDK" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "EXTRACT_ONLY" /D "EXCLUDE_COM" /D "NO_REGISTRY" /D "FORMAT_7Z" /D "COMPRESS_LZMA" /D "COMPRESS_BCJ_X86" /D "COMPRESS_COPY" /D "COMPRESS_PPMD" /D "_SFX" /D "CRYPTO_7ZAES" /D "CRYPTO_AES" /Yu"StdAfx.h" /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"C:\Util\7zSfxCon.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SFXCon - Win32 Release"
# Name "SFXCon - Win32 Debug"
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
# Begin Group "Archiver Common"

# PROP Default_Filter ""
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

SOURCE=..\..\Common\ZipRegistryMain.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Common\ZipRegistryMain.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\ZipSettings.h
# End Source File
# End Group
# Begin Group "Console Archiver"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\Console\ArError.h
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

SOURCE=..\..\Console\ListArchive.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\ListArchive.h
# End Source File
# Begin Source File

SOURCE=..\..\Console\MainAr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\UserInputUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Console\UserInputUtils.h
# End Source File
# End Group
# Begin Group "7z"

# PROP Default_Filter ""
# Begin Group "Header Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Common\ItemNameUtils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Archive\Common\ItemNameUtils.h
# End Source File
# End Group
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

SOURCE=..\..\Format\7z\Extract.cpp
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

SOURCE=..\..\Format\7z\ItemInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\MethodInfo.h
# End Source File
# Begin Source File

SOURCE=..\..\Format\7z\RegistryInfo.h
# End Source File
# End Group
# Begin Group "Format Common"

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

SOURCE=..\..\Format\Common\OutStreamWithCRC.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Format\Common\OutStreamWithCRC.h
# End Source File
# End Group
# Begin Group "Compress"

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

SOURCE=..\..\..\Compress\LZ\LZMA\LenCoder.h
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

!IF  "$(CFG)" == "SFXCon - Win32 Release"

# ADD CPP /Fo"Release\PPMD\Decoder.obj"

!ELSEIF  "$(CFG)" == "SFXCon - Win32 Debug"

# ADD CPP /Fo"Debug\PPMD\Decoder.obj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\Compress\PPM\PPMD\Decoder.h
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
# Begin Group "7zAES"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\7zAES\7zAES.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Crypto\Cipher\7zAES\7zAES.h
# End Source File
# End Group
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
# End Group
# Begin Group "Hash"

# PROP Default_Filter ""
# Begin Group "SHA256"

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

SOURCE=..\..\Format\Common\FormatCryptoInterface.h
# End Source File
# End Group
# Begin Group "SDK"

# PROP Default_Filter ""
# Begin Group "Windows"

# PROP Default_Filter ""
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
# Begin Group "Utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\SDK\Util\FilePathAutoRename.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Util\FilePathAutoRename.h
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

SOURCE=..\..\..\..\SDK\Compression\AriBitCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\CopyCoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\CopyCoder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Compression\RangeCoder.h
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

SOURCE=..\..\..\..\SDK\Stream\WindowOut.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Stream\WindowOut.h
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
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

SOURCE=..\..\..\..\SDK\Common\StdOutStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\StdOutStream.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\SDK\Common\String.cpp
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
# End Group
# Begin Source File

SOURCE=.\7z.ico
# End Source File
# Begin Source File

SOURCE=.\Main.cpp
# End Source File
# End Target
# End Project
