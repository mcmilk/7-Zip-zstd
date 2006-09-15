PROG = 7za.dll
DEF_FILE = ../../Archive/Archive.def
LIBS = $(LIBS) user32.lib oleaut32.lib
CFLAGS = $(CFLAGS) -I ../../../ \
  -DEXCLUDE_COM \
  -DNO_REGISTRY \
  -DFORMAT_7Z \
  -DCOMPRESS_MT \
  -DCOMPRESS_BCJ_X86 \
  -DCOMPRESS_BCJ2 \
  -DCOMPRESS_BZIP2_DECODER \
  -DCOMPRESS_BZIP2_MT \
  -DCOMPRESS_COPY \
  -DCOMPRESS_DEFLATE_DECODER \
  -DCOMPRESS_LZMA \
  -DCOMPRESS_MF_MT \
  -DCOMPRESS_PPMD \
  -DCRYPTO_7ZAES \
  -DCRYPTO_AES \

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
  $O\LSBFDecoder.obj \
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

BZIP2_OBJS = \
  $O\BZip2CRC.obj \

BZIP2_OPT_OBJS = \
  $O\BZip2Decoder.obj \

DEFLATE_OPT_OBJS = \
  $O\DeflateDecoder.obj \

LZ_OBJS = \
  $O\LZInWindow.obj \
  $O\LZOutWindow.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMAEncoder.obj \

PPMD_OPT_OBJS = \
  $O\PPMDDecoder.obj \
  $O\PPMDEncoder.obj \


7ZAES_OPT_OBJS = \
  $O\7zAES.obj \

AES_OPT_OBJS = \
  $O\MyAES.obj \

AES_ORIG_OBJS = \
  $O\aescrypt.obj \
  $O\aeskey.obj \
  $O\aestab.obj \

CRYPTO_HASH_OBJS = \
  $O\Sha256.obj \

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
  $O\CopyCoder.obj \
  $O\MT.obj \
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
$(BZIP2_OBJS): ../../Compress/BZip2/$(*B).cpp
	$(COMPL)
$(BZIP2_OPT_OBJS): ../../Compress/BZip2/$(*B).cpp
	$(COMPL_O2)
$(DEFLATE_OPT_OBJS): ../../Compress/Deflate/$(*B).cpp
	$(COMPL_O2)
$(LZ_OBJS): ../../Compress/LZ/$(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): ../../Compress/LZMA/$(*B).cpp
	$(COMPL_O2)
$(PPMD_OPT_OBJS): ../../Compress/PPMD/$(*B).cpp
	$(COMPL_O2)

$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$O\MT.obj: ../../Compress/LZ/MT/$(*B).cpp
	$(COMPL_O2)
$O\RangeCoderBit.obj: ../../Compress/RangeCoder/$(*B).cpp
	$(COMPL)

$(AES_OPT_OBJS): ../../Crypto/AES/$(*B).cpp
	$(COMPL_O2)
$(AES_ORIG_OBJS): ../../Crypto/AES/$(*B).c
	$(COMPL_O2_W3)
$(7ZAES_OPT_OBJS): ../../Crypto/7zAES/$(*B).cpp
	$(COMPL_O2)
$(CRYPTO_HASH_OBJS): ../../Crypto/Hash/$(*B).cpp
	$(COMPL_O2)
