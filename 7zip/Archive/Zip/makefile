PROG = zip.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

ZIP_OBJS = \
  $O\DllExports.obj \
  $O\ZipAddCommon.obj \
  $O\ZipHandler.obj \
  $O\ZipHandlerOut.obj \
  $O\ZipHeader.obj \
  $O\ZipIn.obj \
  $O\ZipItem.obj \
  $O\ZipOut.obj \
  $O\ZipUpdate.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \
  $O\Random.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\FileFind.obj \
  $O\PropVariant.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\LimitedStreams.obj \
  $O\LSBFDecoder.obj \
  $O\OffsetStream.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamObjects.obj \

AR_COMMON_OBJS = \
  $O\CodecsPath.obj \
  $O\CoderLoader.obj \
  $O\FilterCoder.obj \
  $O\InStreamWithCRC.obj \
  $O\ItemNameUtils.obj \
  $O\OutStreamWithCRC.obj \

7Z_OBJS = \
  $O\7zMethodID.obj \
  $O\7zMethods.obj \

IMPLODE_OBJS = \
  $O\ImplodeDecoder.obj \
  $O\ImplodeHuffmanDecoder.obj \

SHRINK_OBJS = \
  $O\ShrinkDecoder.obj \

CRYPTO_ZIP_OBJS = \
  $O\ZipCipher.obj \
  $O\ZipCrypto.obj \


OBJS = \
  $O\StdAfx.obj \
  $(ZIP_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(CRYPTO_ZIP_OBJS) \
  $(IMPLODE_OBJS) \
  $(SHRINK_OBJS) \
  $O\CopyCoder.obj \
  $O\LZOutWindow.obj \
  $O\resource.res

!include "../../../Build.mak"

$(ZIP_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(7Z_OBJS): ../7z/$(*B).cpp
	$(COMPL)
$(CRYPTO_ZIP_OBJS): ../../Crypto/Zip/$(*B).cpp
	$(COMPL)
$(IMPLODE_OBJS): ../../Compress/Implode/$(*B).cpp
	$(COMPL)
$(SHRINK_OBJS): ../../Compress/Shrink/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$O\LZOutWindow.obj: ../../Compress/LZ/$(*B).cpp
	$(COMPL)
