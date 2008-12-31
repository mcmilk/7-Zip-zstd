PROG = 7za.exe
LIBS = $(LIBS) user32.lib oleaut32.lib Advapi32.lib

CFLAGS = $(CFLAGS) -I ../../../ \
  -D_NO_CRYPTO \
  -DWIN_LONG_PATH \
  -DCOMPRESS_MT \
  -DCOMPRESS_MF_MT \
  -D_NO_CRYPTO \
  -DBREAK_HANDLER \
  -DBENCH_MT \


CONSOLE_OBJS = \
  $O\ConsoleClose.obj \
  $O\ExtractCallbackConsole.obj \
  $O\List.obj \
  $O\Main.obj \
  $O\MainAr.obj \
  $O\OpenCallbackConsole.obj \
  $O\PercentPrinter.obj \
  $O\UpdateCallbackConsole.obj \
  $O\UserInputUtils.obj \

COMMON_OBJS = \
  $O\CommandLineParser.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\ListFileUtils.obj \
  $O\NewHandler.obj \
  $O\StdInStream.obj \
  $O\StdOutStream.obj \
  $O\MyString.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\UTFConvert.obj \
  $O\MyVector.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\Error.obj \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\FileName.obj \
  $O\MemoryLock.obj \
  $O\PropVariant.obj \
  $O\PropVariantConversions.obj \
  $O\Synchronization.obj \
  $O\System.obj \
  $O\Time.obj \

7ZIP_COMMON_OBJS = \
  $O\CreateCoder.obj \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\InBuffer.obj \
  $O\InOutTempBuffer.obj \
  $O\FilterCoder.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\MethodId.obj \
  $O\MethodProps.obj \
  $O\OffsetStream.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \
  $O\VirtThread.obj \

UI_COMMON_OBJS = \
  $O\ArchiveCommandLine.obj \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveOpenCallback.obj \
  $O\DefaultName.obj \
  $O\EnumDirItems.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
  $O\LoadCodecs.obj \
  $O\OpenArchive.obj \
  $O\PropIDUtils.obj \
  $O\SetProperties.obj \
  $O\SortUtils.obj \
  $O\TempFiles.obj \
  $O\Update.obj \
  $O\UpdateAction.obj \
  $O\UpdateCallback.obj \
  $O\UpdatePair.obj \
  $O\UpdateProduce.obj \
  $O\WorkDir.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\DummyOutStream.obj \
  $O\HandlerOut.obj \
  $O\InStreamWithCRC.obj \
  $O\ItemNameUtils.obj \
  $O\MultiStream.obj \
  $O\OutStreamWithCRC.obj \
  $O\ParseProperties.obj \


7Z_OBJS = \
  $O\7zCompressionMode.obj \
  $O\7zDecode.obj \
  $O\7zEncode.obj \
  $O\7zExtract.obj \
  $O\7zFolderInStream.obj \
  $O\7zFolderOutStream.obj \
  $O\7zHandler.obj \
  $O\7zHandlerOut.obj \
  $O\7zHeader.obj \
  $O\7zIn.obj \
  $O\7zOut.obj \
  $O\7zProperties.obj \
  $O\7zRegister.obj \
  $O\7zSpecStream.obj \
  $O\7zUpdate.obj \

LZM_OBJS = \
  $O\LzmaArcRegister.obj \
  $O\LzmaFiltersDecode.obj \
  $O\LzmaHandler.obj \
  $O\LzmaIn.obj \

SPLIT_OBJS = \
  $O\SplitHandler.obj \
  $O\SplitHandlerOut.obj \
  $O\SplitRegister.obj \

COMPRESS_OBJS = \
  $O\Bcj2Coder.obj \
  $O\Bcj2Register.obj \
  $O\BcjCoder.obj \
  $O\BcjRegister.obj \
  $O\BranchCoder.obj \
  $O\BranchMisc.obj \
  $O\BranchRegister.obj \
  $O\ByteSwap.obj \
  $O\ByteSwapRegister.obj \
  $O\CopyCoder.obj \
  $O\CopyRegister.obj \
  $O\LzmaDecoder.obj \
  $O\LzmaEncoder.obj \
  $O\LzmaRegister.obj \

LZMA_BENCH_OBJS = \
  $O\LzmaBench.obj \
  $O\LzmaBenchCon.obj \

C_OBJS = \
  $O\7zCrc.obj \
  $O\Bra.obj \
  $O\Bra86.obj \
  $O\BraIA64.obj \
  $O\Alloc.obj \
  $O\LzmaDec.obj \
  $O\LzmaEnc.obj \
  $O\LzFind.obj \
  $O\LzFindMt.obj \
  $O\Threads.obj \

OBJS = \
  $O\StdAfx.obj \
  $(CONSOLE_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(LZM_OBJS) \
  $(SPLIT_OBJS) \
  $(COMPRESS_OBJS) \
  $(LZMA_BENCH_OBJS) \
  $(C_OBJS) \
  $(CRC_OBJS) \
  $O\resource.res


!include "../../../Build.mak"

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
$(LZM_OBJS): ../../Archive/Lzma/$(*B).cpp
	$(COMPL)
$(SPLIT_OBJS): ../../Archive/Split/$(*B).cpp
	$(COMPL)
$(COMPRESS_OBJS): ../../Compress/$(*B).cpp
	$(COMPL_O2)
$(LZMA_BENCH_OBJS): ../../Compress/LZMA_Alone/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
