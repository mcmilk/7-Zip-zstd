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
  $O\MyMap.obj \
  $O\MyString.obj \
  $O\MyXml.obj \
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
  $O\PropVariantUtils.obj \
  $O\Synchronization.obj \
  $O\System.obj \
  $O\Time.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\InOutTempBuffer.obj \
  $O\CreateCoder.obj \
  $O\FilterCoder.obj \
  $O\LimitedStreams.obj \
  $O\LockedStream.obj \
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
  $O\ArjHandler.obj \
  $O\DebHandler.obj \
  $O\DmgHandler.obj \
  $O\ElfHandler.obj \
  $O\MachoHandler.obj \
  $O\MubHandler.obj \
  $O\PeHandler.obj \
  $O\RpmHandler.obj \
  $O\XarHandler.obj \
  $O\ZHandler.obj \

AR_COMMON_OBJS = \
  $O\CoderMixer2.obj \
  $O\CoderMixer2MT.obj \
  $O\CrossThreadProgress.obj \
  $O\DummyOutStream.obj \
  $O\FindSignature.obj \
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

GZ_OBJS = \
  $O\GZipHandler.obj \
  $O\GZipHandlerOut.obj \
  $O\GZipHeader.obj \
  $O\GZipIn.obj \
  $O\GZipOut.obj \
  $O\GZipUpdate.obj \
  $O\GZipRegister.obj \

HFS_OBJS = \
  $O\HfsHandler.obj \
  $O\HfsIn.obj \
  $O\HfsRegister.obj \

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

LZM_OBJS = \
  $O\LzmaArcRegister.obj \
  $O\LzmaFiltersDecode.obj \
  $O\LzmaHandler.obj \
  $O\LzmaIn.obj \

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

UDF_OBJS = \
  $O\UdfHandler.obj \
  $O\UdfIn.obj \
  $O\UdfRegister.obj \

WIM_OBJS = \
  $O\WimHandler.obj \
  $O\WimIn.obj \
  $O\WimRegister.obj \

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
  $O\ArjDecoder1.obj \
  $O\ArjDecoder2.obj \
  $O\Bcj2Coder.obj \
  $O\Bcj2Register.obj \
  $O\BcjCoder.obj \
  $O\BcjRegister.obj \
  $O\BitlDecoder.obj \
  $O\BranchCoder.obj \
  $O\BranchMisc.obj \
  $O\BranchRegister.obj \
  $O\ByteSwap.obj \
  $O\ByteSwapRegister.obj \
  $O\BZip2Crc.obj \
  $O\BZip2Decoder.obj \
  $O\BZip2Encoder.obj \
  $O\BZip2Register.obj \
  $O\CopyCoder.obj \
  $O\CopyRegister.obj \
  $O\Deflate64Register.obj \
  $O\DeflateDecoder.obj \
  $O\DeflateEncoder.obj \
  $O\DeflateNsisRegister.obj \
  $O\DeflateRegister.obj \
  $O\ImplodeDecoder.obj \
  $O\ImplodeHuffmanDecoder.obj \
  $O\LzhDecoder.obj \
  $O\LzmaDecoder.obj \
  $O\LzmaEncoder.obj \
  $O\LzmaRegister.obj \
  $O\LzOutWindow.obj \
  $O\Lzx86Converter.obj \
  $O\LzxDecoder.obj \
  $O\PpmdDecoder.obj \
  $O\PpmdEncoder.obj \
  $O\PpmdRegister.obj \
  $O\QuantumDecoder.obj \
  $O\Rar1Decoder.obj \
  $O\Rar2Decoder.obj \
  $O\Rar3Decoder.obj \
  $O\Rar3Vm.obj \
  $O\RarCodecsRegister.obj \
  $O\ShrinkDecoder.obj \
  $O\ZlibDecoder.obj \
  $O\ZDecoder.obj \

CRYPTO_OBJS = \
  $O\7zAes.obj \
  $O\7zAesRegister.obj \
  $O\HmacSha1.obj \
  $O\MyAes.obj \
  $O\Pbkdf2HmacSha1.obj \
  $O\RandGen.obj \
  $O\Rar20Crypto.obj \
  $O\RarAes.obj \
  $O\Sha1.obj \
  $O\WzAes.obj \
  $O\ZipCrypto.obj \
  $O\ZipStrong.obj \


C_OBJS = \
  $O\Alloc.obj \
  $O\Bra.obj \
  $O\Bra86.obj \
  $O\BraIA64.obj \
  $O\BwtSort.obj \
  $O\HuffEnc.obj \
  $O\LzFind.obj \
  $O\LzFindMt.obj \
  $O\LzmaDec.obj \
  $O\LzmaEnc.obj \
  $O\Sort.obj \
  $O\Threads.obj \
  $O\Aes.obj \
  $O\Sha256.obj \

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
  $(BZ2_OBJS) \
  $(CAB_OBJS) \
  $(CHM_OBJS) \
  $(COM_OBJS) \
  $(CPIO_OBJS) \
  $(GZ_OBJS) \
  $(HFS_OBJS) \
  $(ISO_OBJS) \
  $(LZH_OBJS) \
  $(LZM_OBJS) \
  $(NSIS_OBJS) \
  $(RAR_OBJS) \
  $(SPLIT_OBJS) \
  $(TAR_OBJS) \
  $(UDF_OBJS) \
  $(WIM_OBJS) \
  $(ZIP_OBJS) \
  $(COMPRESS_OBJS) \
  $(CRYPTO_OBJS) \
  $(C_OBJS) \
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
$(GZ_OBJS): ../../Archive/GZip/$(*B).cpp
	$(COMPL)
$(HFS_OBJS): ../../Archive/Hfs/$(*B).cpp
	$(COMPL)
$(ISO_OBJS): ../../Archive/Iso/$(*B).cpp
	$(COMPL)
$(LZH_OBJS): ../../Archive/Lzh/$(*B).cpp
	$(COMPL)
$(LZM_OBJS): ../../Archive/Lzma/$(*B).cpp
	$(COMPL)
$(NSIS_OBJS): ../../Archive/Nsis/$(*B).cpp
	$(COMPL)
$(RAR_OBJS): ../../Archive/Rar/$(*B).cpp
	$(COMPL)
$(SPLIT_OBJS): ../../Archive/Split/$(*B).cpp
	$(COMPL)
$(TAR_OBJS): ../../Archive/Tar/$(*B).cpp
	$(COMPL)
$(UDF_OBJS): ../../Archive/Udf/$(*B).cpp
	$(COMPL)
$(WIM_OBJS): ../../Archive/Wim/$(*B).cpp
	$(COMPL)
$(ZIP_OBJS): ../../Archive/Zip/$(*B).cpp
	$(COMPL)

$(COMPRESS_OBJS): ../../Compress/$(*B).cpp
	$(COMPL_O2)

$(CRYPTO_OBJS): ../../Crypto/$(*B).cpp
	$(COMPL_O2)

$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)

!include "../../Crc.mak"
