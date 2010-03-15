PROG = 7zCon.sfx
MY_CONSOLE = 1
CFLAGS = $(CFLAGS) -I ../../../ \
  -DEXTRACT_ONLY \
  -DNO_READ_FROM_CODER \
  -D_SFX \

SFX_CONSOLE_OBJS = \
  $O\Main.obj \

CONSOLE_OBJS = \
  $O\ConsoleClose.obj \
  $O\ExtractCallbackConsole.obj \
  $O\List.obj \
  $O\MainAr.obj \
  $O\OpenCallbackConsole.obj \
  $O\UserInputUtils.obj \

COMMON_OBJS = \
  $O\CommandLineParser.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\StdInStream.obj \
  $O\StdOutStream.obj \
  $O\MyString.obj \
  $O\StringConvert.obj \
  $O\MyVector.obj \
  $O\Wildcard.obj \
  $O\UTFConvert.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\Error.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\FileName.obj \
  $O\PropVariant.obj \
  $O\PropVariantConversions.obj \
  $O\Synchronization.obj \

7ZIP_COMMON_OBJS = \
  $O\CreateCoder.obj \
  $O\CWrappers.obj \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\InBuffer.obj \
  $O\FilterCoder.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \
  $O\VirtThread.obj \

UI_COMMON_OBJS = \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveOpenCallback.obj \
  $O\DefaultName.obj \
  $O\LoadCodecs.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
  $O\OpenArchive.obj \
  $O\PropIDUtils.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\ItemNameUtils.obj \
  $O\OutStreamWithCRC.obj \


7Z_OBJS = \
  $O\7zDecode.obj \
  $O\7zExtract.obj \
  $O\7zFolderOutStream.obj \
  $O\7zHandler.obj \
  $O\7zHeader.obj \
  $O\7zIn.obj \
  $O\7zRegister.obj \

COMPRESS_OBJS = \
  $O\BranchCoder.obj \
  $O\Bcj2Coder.obj \
  $O\Bcj2Register.obj \
  $O\BcjCoder.obj \
  $O\BcjRegister.obj \
  $O\CopyCoder.obj \
  $O\CopyRegister.obj \
  $O\Lzma2Decoder.obj \
  $O\Lzma2Register.obj \
  $O\LzmaDecoder.obj \
  $O\LzmaRegister.obj \
  $O\PpmdDecoder.obj \
  $O\PpmdRegister.obj \

CRYPTO_OBJS = \
  $O\7zAes.obj \
  $O\7zAesRegister.obj \
  $O\MyAes.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\Bra86.obj \
  $O\CpuArch.obj \
  $O\Lzma2Dec.obj \
  $O\LzmaDec.obj \
  $O\Ppmd7.obj \
  $O\Ppmd7Dec.obj \
  $O\Sha256.obj \
  $O\Threads.obj \

!include "../../Aes.mak"
!include "../../Crc.mak"

OBJS = \
  $O\StdAfx.obj \
  $(SFX_CONSOLE_OBJS) \
  $(CONSOLE_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(COMPRESS_OBJS) \
  $(CRYPTO_OBJS) \
  $(C_OBJS) \
  $(ASM_OBJS) \
  $O\resource.res


!include "../../../Build.mak"

$(SFX_CONSOLE_OBJS): $(*B).cpp
	$(COMPL)

$(CONSOLE_OBJS): ../../UI/Console/$(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(UI_COMMON_OBJS): ../../UI/Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../../Archive/Common/$(*B).cpp
	$(COMPL)

$(7Z_OBJS): ../../Archive/7z/$(*B).cpp
	$(COMPL)
$(COMPRESS_OBJS): ../../Compress/$(*B).cpp
	$(COMPL)
$(CRYPTO_OBJS): ../../Crypto/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)

!include "../../Asm.mak"
