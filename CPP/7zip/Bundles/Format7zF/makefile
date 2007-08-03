PROG = 7z.dll
DEF_FILE = ../../Archive/Archive2.def
LIBS = $(LIBS) user32.lib oleaut32.lib
CFLAGS = $(CFLAGS) -I ../../../ \
  -DEXTERNAL_CODECS \
  -DCOMPRESS_MT \
  -DCOMPRESS_BZIP2_MT \
  -DCOMPRESS_MF_MT \
  -D_7ZIP_LARGE_PAGES \

COMMON_OBJS = \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\MyString.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\MyVector.obj \
  $O\UTFConvert.obj \
  $O\Wildcard.obj \

WIN_OBJS = \
  $O\FileDir.obj \
  $O\FileFind.obj \
  $O\FileIO.obj \
  $O\PropVariant.obj \
  $O\Synchronization.obj \
  $O\System.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\InOutTempBuffer.obj \
  $O\CreateCoder.obj \
  $O\FilterCoder.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
  $O\LSBFDecoder.obj \
  $O\LSBFEncoder.obj \
  $O\MethodId.obj \
  $O\MethodProps.obj \
  $O\MemBlocks.obj \
  $O\OffsetStream.obj \
  $O\OutBuffer.obj \
  $O\OutMemStream.obj \
  $O\ProgressMt.obj \
  $O\ProgressUtils.obj \
  $O\StreamBinder.obj \
  $O\StreamObjects.obj \
  $O\StreamUtils.obj \
  $O\VirtThread.obj \

AR_OBJS = \
  $O\ArchiveExports.obj \
  $O\DllExports2.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\DummyOutStream.obj \
  $O\InStreamWithCRC.obj \
  $O\ItemNameUtils.obj \
  $O\MultiStream.obj \
  $O\OutStreamWithCRC.obj \
  $O\OutStreamWithSha1.obj \
  $O\HandlerOut.obj \
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
  $O\7zSpecStream.obj \
  $O\7zUpdate.obj \
  $O\7zRegister.obj \

ARJ_OBJS = \
  $O\ArjHandler.obj \
  $O\ArjIn.obj \
  $O\ArjRegister.obj \

BZ2_OBJS = \
  $O\BZip2Handler.obj \
  $O\BZip2HandlerOut.obj \
  $O\BZip2Update.obj \
  $O\bz2Register.obj \

CAB_OBJS = \
  $O\CabBlockInStream.obj \
  $O\CabHandler.obj \
  $O\CabHeader.obj \
  $O\CabIn.obj \
  $O\CabRegister.obj \

CHM_OBJS = \
  $O\ChmHandler.obj \
  $O\ChmHeader.obj \
  $O\ChmIn.obj \
  $O\ChmRegister.obj \

COM_OBJS = \
  $O\ComHandler.obj \
  $O\ComIn.obj \
  $O\ComRegister.obj \

CPIO_OBJS = \
  $O\CpioHandler.obj \
  $O\CpioHeader.obj \
  $O\CpioIn.obj \
  $O\CpioRegister.obj \

DEB_OBJS = \
  $O\DebHandler.obj \
  $O\DebHeader.obj \
  $O\DebIn.obj \
  $O\DebRegister.obj \

GZ_OBJS = \
  $O\GZipHandler.obj \
  $O\GZipHandlerOut.obj \
  $O\GZipHeader.obj \
  $O\GZipIn.obj \
  $O\GZipOut.obj \
  $O\GZipUpdate.obj \
  $O\GZipRegister.obj \

ISO_OBJS = \
  $O\IsoHandler.obj \
  $O\IsoHeader.obj \
  $O\IsoIn.obj \
  $O\IsoRegister.obj \

LZH_OBJS = \
  $O\LzhCRC.obj \
  $O\LzhHandler.obj \
  $O\LzhIn.obj \
  $O\LzhOutStreamWithCRC.obj \
  $O\LzhRegister.obj \

NSIS_OBJS = \
  $O\NsisDecode.obj \
  $O\NsisHandler.obj \
  $O\NsisIn.obj \
  $O\NsisRegister.obj \

RAR_OBJS = \
  $O\RarHandler.obj \
  $O\RarHeader.obj \
  $O\RarIn.obj \
  $O\RarItem.obj \
  $O\RarVolumeInStream.obj \
  $O\RarRegister.obj \

RPM_OBJS = \
  $O\RpmHandler.obj \
  $O\RpmIn.obj \
  $O\RpmRegister.obj \

SPLIT_OBJS = \
  $O\SplitHandler.obj \
  $O\SplitHandlerOut.obj \
  $O\SplitRegister.obj \

TAR_OBJS = \
  $O\TarHandler.obj \
  $O\TarHandlerOut.obj \
  $O\TarHeader.obj \
  $O\TarIn.obj \
  $O\TarOut.obj \
  $O\TarUpdate.obj \
  $O\TarRegister.obj \

WIM_OBJS = \
  $O\WimHandler.obj \
  $O\WimIn.obj \
  $O\WimRegister.obj \

Z_OBJS = \
  $O\ZHandler.obj \
  $O\ZRegister.obj \

ZIP_OBJS = \
  $O\ZipAddCommon.obj \
  $O\ZipHandler.obj \
  $O\ZipHandlerOut.obj \
  $O\ZipHeader.obj \
  $O\ZipIn.obj \
  $O\ZipItem.obj \
  $O\ZipOut.obj \
  $O\ZipUpdate.obj \
  $O\ZipRegister.obj \

COMPRESS_OBJS = \
  $O\CodecExports.obj \

BRANCH_OPT_OBJS = \
  $O\BranchCoder.obj \
  $O\BranchRegister.obj \
  $O\BCJRegister.obj \
  $O\BCJ2Register.obj \
  $O\x86.obj \
  $O\x86_2.obj \
  $O\ARM.obj \
  $O\ARMThumb.obj \
  $O\IA64.obj \
  $O\PPC.obj \
  $O\SPARC.obj \

SWAP_OPT_OBJS = \
  $O\ByteSwap.obj \
  $O\ByteSwapRegister.obj \

BZIP2_OBJS = \
  $O\BZip2CRC.obj \
  $O\BZip2Register.obj \

BZIP2_OPT_OBJS = \
  $O\BZip2Decoder.obj \
  $O\BZip2Encoder.obj \

COPY_OBJS = \
  $O\CopyCoder.obj \
  $O\CopyRegister.obj \

DEFLATE_OPT_OBJS = \
  $O\DeflateDecoder.obj \
  $O\DeflateEncoder.obj \
  $O\DeflateRegister.obj \
  $O\Deflate64Register.obj \
  $O\DeflateNsisRegister.obj \

IMPLODE_OBJS = \
  $O\ImplodeDecoder.obj \
  $O\ImplodeHuffmanDecoder.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMAEncoder.obj \
  $O\LZMARegister.obj \

LZX_OBJS = \
  $O\LzxDecoder.obj \
  $O\Lzx86Converter.obj \

PPMD_OPT_OBJS = \
  $O\PPMDDecoder.obj \
  $O\PPMDEncoder.obj \
  $O\PPMDRegister.obj \

RAR29_OPT_OBJS = \
  $O\Rar1Decoder.obj \
  $O\Rar2Decoder.obj \
  $O\Rar3Decoder.obj \
  $O\Rar3Vm.obj \
  $O\RarCodecsRegister.obj \

SHRINK_OBJS = \
  $O\ShrinkDecoder.obj \

COMPRESS_ARJ_OBJS = \
  $O\ArjDecoder1.obj \
  $O\ArjDecoder2.obj \

COMPRESS_LZH_OBJS = \
  $O\LzhDecoder.obj \

COMPRESS_Z_OBJS = \
  $O\ZDecoder.obj \

7ZAES_OPT_OBJS = \
  $O\7zAES.obj \
  $O\7zAESRegister.obj \

AES_OPT_OBJS = \
  $O\MyAES.obj \

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

CRYPTO_RAR20_OBJS = \
  $O\Rar20Cipher.obj \
  $O\Rar20Crypto.obj \

CRYPTO_RARAES_OBJS = \
  $O\RarAES.obj \


C_OBJS = \
  $O\Alloc.obj \
  $O\Threads.obj \
  $O\Sort.obj \

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

C_CRYPTO = \
  $O\Aes.obj \

!include "../../Crc2.mak"

OBJS = \
  $O\StdAfx.obj \
  $(CONSOLE_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(ARJ_OBJS) \
  $(BZ2_OBJS) \
  $(CAB_OBJS) \
  $(CHM_OBJS) \
  $(COM_OBJS) \
  $(CPIO_OBJS) \
  $(DEB_OBJS) \
  $(GZ_OBJS) \
  $(ISO_OBJS) \
  $(LZH_OBJS) \
  $(NSIS_OBJS) \
  $(RAR_OBJS) \
  $(RPM_OBJS) \
  $(SPLIT_OBJS) \
  $(TAR_OBJS) \
  $(WIM_OBJS) \
  $(Z_OBJS) \
  $(ZIP_OBJS) \
  $(BZIP2_OBJS) \
  $(BZIP2_OPT_OBJS) \
  $(COMPRESS_OBJS) \
  $(BRANCH_OPT_OBJS) \
  $(SWAP_OPT_OBJS) \
  $(COPY_OBJS) \
  $(DEFLATE_OPT_OBJS) \
  $(IMPLODE_OBJS) \
  $(LZ_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(LZX_OBJS) \
  $(PPMD_OPT_OBJS) \
  $(RAR29_OPT_OBJS) \
  $(SHRINK_OBJS) \
  $(COMPRESS_ARJ_OBJS) \
  $(COMPRESS_LZH_OBJS) \
  $(COMPRESS_Z_OBJS) \
  $O\QuantumDecoder.obj \
  $(C_OBJS) \
  $(C_LZ_OBJS) \
  $(C_BRANCH_OBJS) \
  $(C_CRYPTO) \
  $O\HuffmanEncode.obj \
  $O\BlockSort.obj \
  $O\RangeCoderBit.obj \
  $(7ZAES_OPT_OBJS) \
  $(AES_OPT_OBJS) \
  $(CRYPTO_ZIP_OBJS) \
  $(CRYPTO_WZAES_OBJS) \
  $(CRYPTO_HASH_OBJS) \
  $(CRYPTO_RAR20_OBJS) \
  $(CRYPTO_RARAES_OBJS) \
  $(CRC_OBJS) \
  $O\resource.res


!include "../../../Build.mak"

$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_OBJS): ../../Archive/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../../Archive/Common/$(*B).cpp
	$(COMPL)

$(7Z_OBJS): ../../Archive/7z/$(*B).cpp
	$(COMPL)
$(ARJ_OBJS): ../../Archive/Arj/$(*B).cpp
	$(COMPL)
$(BZ2_OBJS): ../../Archive/BZip2/$(*B).cpp
	$(COMPL)
$(CAB_OBJS): ../../Archive/Cab/$(*B).cpp
	$(COMPL)
$(CHM_OBJS): ../../Archive/Chm/$(*B).cpp
	$(COMPL)
$(COM_OBJS): ../../Archive/Com/$(*B).cpp
	$(COMPL)
$(CPIO_OBJS): ../../Archive/Cpio/$(*B).cpp
	$(COMPL)
$(DEB_OBJS): ../../Archive/Deb/$(*B).cpp
	$(COMPL)
$(GZ_OBJS): ../../Archive/GZip/$(*B).cpp
	$(COMPL)
$(ISO_OBJS): ../../Archive/Iso/$(*B).cpp
	$(COMPL)
$(LZH_OBJS): ../../Archive/Lzh/$(*B).cpp
	$(COMPL)
$(NSIS_OBJS): ../../Archive/Nsis/$(*B).cpp
	$(COMPL)
$(RAR_OBJS): ../../Archive/Rar/$(*B).cpp
	$(COMPL)
$(RPM_OBJS): ../../Archive/Rpm/$(*B).cpp
	$(COMPL)
$(SPLIT_OBJS): ../../Archive/Split/$(*B).cpp
	$(COMPL)
$(TAR_OBJS): ../../Archive/Tar/$(*B).cpp
	$(COMPL)
$(WIM_OBJS): ../../Archive/Wim/$(*B).cpp
	$(COMPL)
$(Z_OBJS): ../../Archive/Z/$(*B).cpp
	$(COMPL)
$(ZIP_OBJS): ../../Archive/Zip/$(*B).cpp
	$(COMPL)

$(COMPRESS_OBJS): ../../Compress/$(*B).cpp
	$(COMPL)
$(BRANCH_OPT_OBJS): ../../Compress/Branch/$(*B).cpp
	$(COMPL_O2)
$(SWAP_OPT_OBJS): ../../Compress/ByteSwap/$(*B).cpp
	$(COMPL_O2)
$(BZIP2_OBJS): ../../Compress/BZip2/$(*B).cpp
	$(COMPL)
$(BZIP2_OPT_OBJS): ../../Compress/BZip2/$(*B).cpp
	$(COMPL_O2)
$(COPY_OBJS): ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$(DEFLATE_OPT_OBJS): ../../Compress/Deflate/$(*B).cpp
	$(COMPL_O2)
$(IMPLODE_OBJS): ../../Compress/Implode/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../../Compress/LZ/$(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): ../../Compress/LZMA/$(*B).cpp
	$(COMPL_O2)
$(LZX_OBJS): ../../Compress/Lzx/$(*B).cpp
	$(COMPL_O2)
$(PPMD_OPT_OBJS): ../../Compress/PPMD/$(*B).cpp
	$(COMPL_O2)
$(RAR29_OPT_OBJS): ../../Compress/Rar/$(*B).cpp
	$(COMPL_O2)
$(SHRINK_OBJS): ../../Compress/Shrink/$(*B).cpp
	$(COMPL)
$(COMPRESS_ARJ_OBJS): ../../Compress/Arj/$(*B).cpp
	$(COMPL)
$(COMPRESS_LZH_OBJS): ../../Compress/Lzh/$(*B).cpp
	$(COMPL)
$(COMPRESS_Z_OBJS): ../../Compress/Z/$(*B).cpp
	$(COMPL)
$O\QuantumDecoder.obj: ../../Compress/Quantum/$(*B).cpp
	$(COMPL)

$O\BlockSort.obj: ../../Compress/BWT/$(*B).cpp
	$(COMPL_O2)
$O\RangeCoderBit.obj: ../../Compress/RangeCoder/$(*B).cpp
	$(COMPL)

$(AES_OPT_OBJS): ../../Crypto/AES/$(*B).cpp
	$(COMPL_O2)
$(7ZAES_OPT_OBJS): ../../Crypto/7zAES/$(*B).cpp
	$(COMPL_O2)
$(CRYPTO_ZIP_OBJS): ../../Crypto/Zip/$(*B).cpp
	$(COMPL)
$(CRYPTO_WZAES_OBJS): ../../Crypto/WzAES/$(*B).cpp
	$(COMPL_O2)
$(CRYPTO_HASH_OBJS): ../../Crypto/Hash/$(*B).cpp
	$(COMPL_O2)
$(CRYPTO_RAR20_OBJS): ../../Crypto/Rar20/$(*B).cpp
	$(COMPL)
$(CRYPTO_RARAES_OBJS): ../../Crypto/RarAES/$(*B).cpp
	$(COMPL)

$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
$(C_LZ_OBJS): ../../../../C/Compress/Lz/$(*B).c
	$(COMPL_O2)
$(C_BRANCH_OBJS): ../../../../C/Compress/Branch/$(*B).c
	$(COMPL_O2)
$O\HuffmanEncode.obj: ../../../../C/Compress/Huffman/$(*B).c
	$(COMPL_O2)
$(C_CRYPTO): ../../../../C/Crypto/$(*B).c
	$(COMPL_O2)

!include "../../Crc.mak"
