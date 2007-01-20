PROG = 7za.exe
LIBS = $(LIBS) user32.lib oleaut32.lib Advapi32.lib

CFLAGS = $(CFLAGS) -I ../../../ \
  -DEXCLUDE_COM \
  -DNO_REGISTRY \
  -DWIN_LONG_PATH \
  -DFORMAT_7Z \
  -DFORMAT_BZIP2 \
  -DFORMAT_CAB \
  -DFORMAT_GZIP \
  -DFORMAT_SPLIT \
  -DFORMAT_TAR \
  -DFORMAT_Z \
  -DFORMAT_ZIP \
  -DCOMPRESS_MT \
  -DCOMPRESS_BCJ_X86 \
  -DCOMPRESS_BCJ2 \
  -DCOMPRESS_BZIP2 \
  -DCOMPRESS_BZIP2_MT \
  -DCOMPRESS_COPY \
  -DCOMPRESS_DEFLATE \
  -DCOMPRESS_DEFLATE64 \
  -DCOMPRESS_IMPLODE \
  -DCOMPRESS_LZMA \
  -DCOMPRESS_MF_MT \
  -DCOMPRESS_PPMD \
  -DCRYPTO_7ZAES \
  -DCRYPTO_AES \
  -DCRYPTO_ZIP \


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
  $O\Alloc.obj \
  $O\CommandLineParser.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\ListFileUtils.obj \
  $O\NewHandler.obj \
  $O\StdInStream.obj \
  $O\StdOutStream.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\UTFConvert.obj \
  $O\Vector.obj \
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
  $O\Synchronization.obj

7ZIP_COMMON_OBJS = \
  $O\FilePathAutoRename.obj \
  $O\FileStreams.obj \
  $O\InBuffer.obj \
  $O\InOutTempBuffer.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\LSBFDecoder.obj \
  $O\LSBFEncoder.obj \
  $O\MemBlocks.obj \
  $O\OffsetStream.obj \
  $O\OutBuffer.obj \
  $O\OutMemStream.obj \
  $O\ProgressMt.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \

UI_COMMON_OBJS = \
  $O\ArchiveCommandLine.obj \
  $O\ArchiveExtractCallback.obj \
  $O\ArchiveOpenCallback.obj \
  $O\ArchiverInfo.obj \
  $O\DefaultName.obj \
  $O\EnumDirItems.obj \
  $O\Extract.obj \
  $O\ExtractingFilePath.obj \
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
  $O\FilterCoder.obj \
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
  $O\7zMethodID.obj \
  $O\7zOut.obj \
  $O\7zProperties.obj \
  $O\7zSpecStream.obj \
  $O\7zUpdate.obj \

BZ2_OBJS = \
  $O\BZip2Handler.obj \
  $O\BZip2HandlerOut.obj \
  $O\BZip2Update.obj \

CAB_OBJS = \
  $O\CabBlockInStream.obj \
  $O\CabHandler.obj \
  $O\CabHeader.obj \
  $O\CabIn.obj \

GZ_OBJS = \
  $O\GZipHandler.obj \
  $O\GZipHandlerOut.obj \
  $O\GZipHeader.obj \
  $O\GZipIn.obj \
  $O\GZipOut.obj \
  $O\GZipUpdate.obj \

SPLIT_OBJS = \
  $O\SplitHandler.obj \
  $O\SplitHandlerOut.obj \

TAR_OBJS = \
  $O\TarHandler.obj \
  $O\TarHandlerOut.obj \
  $O\TarHeader.obj \
  $O\TarIn.obj \
  $O\TarOut.obj \
  $O\TarUpdate.obj \

Z_OBJS = \
  $O\ZHandler.obj \

ZIP_OBJS = \
  $O\ZipAddCommon.obj \
  $O\ZipHandler.obj \
  $O\ZipHandlerOut.obj \
  $O\ZipHeader.obj \
  $O\ZipIn.obj \
  $O\ZipItem.obj \
  $O\ZipOut.obj \
  $O\ZipUpdate.obj \


BRANCH_OPT_OBJS = \
  $O\BranchCoder.obj \
  $O\x86.obj \
  $O\x86_2.obj \

BZIP2_OBJS = \
  $O\BZip2CRC.obj \

BZIP2_OPT_OBJS = \
  $O\BZip2Decoder.obj \
  $O\BZip2Encoder.obj \

DEFLATE_OPT_OBJS = \
  $O\DeflateDecoder.obj \
  $O\DeflateEncoder.obj \

LZ_OBJS = \
  $O\LzOutWindow.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMAEncoder.obj \

LZX_OBJS = \
  $O\LzxDecoder.obj \
  $O\Lzx86Converter.obj \

IMPLODE_OBJS = \
  $O\ImplodeDecoder.obj \
  $O\ImplodeHuffmanDecoder.obj \

PPMD_OPT_OBJS = \
  $O\PPMDDecoder.obj \
  $O\PPMDEncoder.obj \

SHRINK_OBJS = \
  $O\ShrinkDecoder.obj \

COMPRESS_Z_OBJS = \
  $O\ZDecoder.obj \


7ZAES_OPT_OBJS = \
  $O\7zAES.obj \

AES_OPT_OBJS = \
  $O\MyAES.obj \

AES_ORIG_OBJS = \
  $O\aescrypt.obj \
  $O\aeskey.obj \
  $O\aestab.obj \

CRYPTO_HASH_OBJS = \
  $O\HmacSha1.obj \
  $O\Pbkdf2HmacSha1.obj \
  $O\RandGen.obj \
  $O\Sha1.obj \
  $O\Sha256.obj \

CRYPTO_WZAES_OBJS = \
  $O\WzAES.obj \

CRYPTO_ZIP_OBJS = \
  $O\ZipCipher.obj \
  $O\ZipCrypto.obj \

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
  $(UI_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(BZ2_OBJS) \
  $(CAB_OBJS) \
  $(GZ_OBJS) \
  $(SPLIT_OBJS) \
  $(TAR_OBJS) \
  $(Z_OBJS) \
  $(ZIP_OBJS) \
  $(BZIP2_OBJS) \
  $(BZIP2_OPT_OBJS) \
  $(BRANCH_OPT_OBJS) \
  $(DEFLATE_OPT_OBJS) \
  $(IMPLODE_OBJS) \
  $(LZ_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(LZX_OBJS) \
  $(PPMD_OPT_OBJS) \
  $(SHRINK_OBJS) \
  $(COMPRESS_Z_OBJS) \
  $(C_OBJS) \
  $(C_LZ_OBJS) \
  $O\BlockSort.obj \
  $O\CopyCoder.obj \
  $O\HuffmanEncode.obj \
  $O\RangeCoderBit.obj \
  $(7ZAES_OPT_OBJS) \
  $(AES_OPT_OBJS) \
  $(AES_ORIG_OBJS) \
  $(CRYPTO_HASH_OBJS) \
  $(CRYPTO_ZIP_OBJS) \
  $(CRYPTO_WZAES_OBJS) \
  $O\QuantumDecoder.obj \
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
$(BZ2_OBJS): ../../Archive/BZip2/$(*B).cpp
	$(COMPL)
$(CAB_OBJS): ../../Archive/Cab/$(*B).cpp
	$(COMPL)
$(GZ_OBJS): ../../Archive/GZip/$(*B).cpp
	$(COMPL)
$(SPLIT_OBJS): ../../Archive/Split/$(*B).cpp
	$(COMPL)
$(TAR_OBJS): ../../Archive/Tar/$(*B).cpp
	$(COMPL)
$(Z_OBJS): ../../Archive/Z/$(*B).cpp
	$(COMPL)
$(ZIP_OBJS): ../../Archive/Zip/$(*B).cpp
	$(COMPL)

$(BRANCH_OPT_OBJS): ../../Compress/Branch/$(*B).cpp
	$(COMPL_O2)
$(BZIP2_OBJS): ../../Compress/BZip2/$(*B).cpp
	$(COMPL)
$(BZIP2_OPT_OBJS): ../../Compress/BZip2/$(*B).cpp
	$(COMPL_O2)
$(DEFLATE_OPT_OBJS): ../../Compress/Deflate/$(*B).cpp
	$(COMPL_O2)
$(IMPLODE_OBJS): ../../Compress/Implode/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../../Compress/LZ/$(*B).cpp
	$(COMPL_O2)
$(LZMA_OPT_OBJS): ../../Compress/LZMA/$(*B).cpp
	$(COMPL_O2)
$(LZX_OBJS): ../../Compress/Lzx/$(*B).cpp
	$(COMPL_O2)
$(PPMD_OPT_OBJS): ../../Compress/PPMD/$(*B).cpp
	$(COMPL_O2)
$(SHRINK_OBJS): ../../Compress/Shrink/$(*B).cpp
	$(COMPL)
$(COMPRESS_Z_OBJS): ../../Compress/Z/$(*B).cpp
	$(COMPL)

$O\BlockSort.obj: ../../Compress/BWT/$(*B).cpp
	$(COMPL_O2)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$O\RangeCoderBit.obj: ../../Compress/RangeCoder/$(*B).cpp
	$(COMPL)
$O\QuantumDecoder.obj: ../../Compress/Quantum/$(*B).cpp
	$(COMPL)

$(AES_OPT_OBJS): ../../Crypto/AES/$(*B).cpp
	$(COMPL_O2)
$(AES_ORIG_OBJS): ../../Crypto/AES/$(*B).c
	$(COMPL_O2_W3)
$(7ZAES_OPT_OBJS): ../../Crypto/7zAES/$(*B).cpp
	$(COMPL_O2)
$(CRYPTO_HASH_OBJS): ../../Crypto/Hash/$(*B).cpp
	$(COMPL_O2)
$(CRYPTO_ZIP_OBJS): ../../Crypto/Zip/$(*B).cpp
	$(COMPL)
$(CRYPTO_WZAES_OBJS): ../../Crypto/WzAES/$(*B).cpp
	$(COMPL_O2)

$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
$(C_LZ_OBJS): ../../../../C/Compress/Lz/$(*B).c
	$(COMPL_O2)
$O\HuffmanEncode.obj: ../../../../C/Compress/Huffman/$(*B).c
	$(COMPL_O2)
