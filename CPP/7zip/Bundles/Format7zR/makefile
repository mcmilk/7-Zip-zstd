PROG = 7zra.dll
DEF_FILE = ../../Archive/Archive.def
LIBS = $(LIBS) user32.lib oleaut32.lib
CFLAGS = $(CFLAGS) -I ../../../ \
  -DEXCLUDE_COM \
  -DNO_REGISTRY \
  -DFORMAT_7Z \
  -DCOMPRESS_MT \
  -DCOMPRESS_BCJ_X86 \
  -DCOMPRESS_BCJ2 \
  -DCOMPRESS_COPY \
  -DCOMPRESS_LZMA \
  -DCOMPRESS_MF_MT \
  -D_NO_CRYPTO

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\Vector.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\PropVariant.obj \
  $O\Synchronization.obj

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\InOutTempBuffer.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\FilterCoder.obj \
  $O\InStreamWithCRC.obj \
  $O\ItemNameUtils.obj \
  $O\OutStreamWithCRC.obj \
  $O\ParseProperties.obj \


7Z_OBJS = \
  $O\DllExports.obj \
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
  $O\7zMethodID.obj \
  $O\7zOut.obj \
  $O\7zProperties.obj \
  $O\7zSpecStream.obj \
  $O\7zUpdate.obj \


BRANCH_OPT_OBJS = \
  $O\BranchCoder.obj \
  $O\x86.obj \
  $O\x86_2.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMAEncoder.obj \

C_OBJS = \
  $O\7zCrc.obj \
  $O\Sort.obj \
  $O\Threads.obj \

C_LZ_OBJS = \
  $O\MatchFinder.obj \
  $O\MatchFinderMt.obj \

OBJS = \
  $O\StdAfx.obj \
  $(CONSOLE_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(BZIP2_OBJS) \
  $(BZIP2_OPT_OBJS) \
  $(BRANCH_OPT_OBJS) \
  $(DEFLATE_OPT_OBJS) \
  $(LZ_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(PPMD_OPT_OBJS) \
  $(C_OBJS) \
  $(C_LZ_OBJS) \
  $O\CopyCoder.obj \
  $O\RangeCoderBit.obj \
  $(7ZAES_OPT_OBJS) \
  $(AES_OPT_OBJS) \
  $(AES_ORIG_OBJS) \
  $(CRYPTO_HASH_OBJS) \
  $O\resource.res


!include "../../../Build.mak"

$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../../Archive/Common/$(*B).cpp
	$(COMPL)

$(7Z_OBJS): ../../Archive/7z/$(*B).cpp
	$(COMPL)

$(BRANCH_OPT_OBJS): ../../Compress/Branch/$(*B).cpp
	$(COMPL_O2)
$(LZ_OBJS): ../../Compress/LZ/$(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): ../../Compress/LZMA/$(*B).cpp
	$(COMPL_O2)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$O\RangeCoderBit.obj: ../../Compress/RangeCoder/$(*B).cpp
	$(COMPL)

$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
$(C_LZ_OBJS): ../../../../C/Compress/Lz/$(*B).c
	$(COMPL_O2)
