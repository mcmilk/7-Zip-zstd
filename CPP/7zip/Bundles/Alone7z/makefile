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


BRANCH_OPT_OBJS = \
  $O\BranchCoder.obj \
  $O\x86.obj \
  $O\x86_2.obj \
  $O\ARM.obj \
  $O\ARMThumb.obj \
  $O\IA64.obj \
  $O\PPC.obj \
  $O\SPARC.obj \
  $O\BranchRegister.obj \
  $O\BCJRegister.obj \
  $O\BCJ2Register.obj \

SWAP_OPT_OBJS = \
  $O\ByteSwap.obj \
  $O\ByteSwapRegister.obj \

COPY_OBJS = \
  $O\CopyCoder.obj \
  $O\CopyRegister.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMAEncoder.obj \
  $O\LZMARegister.obj \

LZMA_BENCH_OBJS = \
  $O\LzmaBench.obj \
  $O\LzmaBenchCon.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\7zCrc.obj \
  $O\Sort.obj \
  $O\Threads.obj \

C_LZ_OBJS = \
  $O\MatchFinder.obj \
  $O\MatchFinderMt.obj \

C_BRANCH_OBJS = \
  $O\BranchARM.obj \
  $O\BranchARMThumb.obj \
  $O\BranchIA64.obj \
  $O\BranchPPC.obj \
  $O\BranchSPARC.obj \
  $O\BranchX86.obj \

OBJS = \
  $O\StdAfx.obj \
  $(CONSOLE_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(UI_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(BRANCH_OPT_OBJS) \
  $(SWAP_OPT_OBJS) \
  $(COPY_OBJS) \
  $(LZ_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(LZMA_BENCH_OBJS) \
  $(C_OBJS) \
  $(C_LZ_OBJS) \
  $(C_BRANCH_OBJS) \
  $O\RangeCoderBit.obj \
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
$(BRANCH_OPT_OBJS): ../../Compress/Branch/$(*B).cpp
	$(COMPL_O2)
$(SWAP_OPT_OBJS): ../../Compress/ByteSwap/$(*B).cpp
	$(COMPL_O2)
$(COPY_OBJS): ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../../Compress/LZ/$(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): ../../Compress/LZMA/$(*B).cpp
	$(COMPL_O2)
$(LZMA_BENCH_OBJS): ../../Compress/LZMA_Alone/$(*B).cpp
	$(COMPL)
$O\RangeCoderBit.obj: ../../Compress/RangeCoder/$(*B).cpp
	$(COMPL)

$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
$(C_LZ_OBJS): ../../../../C/Compress/Lz/$(*B).c
	$(COMPL_O2)
$(C_BRANCH_OBJS): ../../../../C/Compress/Branch/$(*B).c
	$(COMPL_O2)
