PROG = rar.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

RAR_OBJS = \
  $O\DllExports.obj \
  $O\RarHandler.obj \
  $O\RarHeader.obj \
  $O\RarIn.obj \
  $O\RarItem.obj \
  $O\RarVolumeInStream.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\FileFind.obj \
  $O\PropVariant.obj \

7ZIP_COMMON_OBJS = \
  $O\LimitedStreams.obj \
  $O\ProgressUtils.obj \
  $O\StreamObjects.obj \

AR_COMMON_OBJS = \
  $O\CodecsPath.obj \
  $O\FilterCoder.obj \
  $O\InStreamWithCRC.obj \
  $O\OutStreamWithCRC.obj \

7Z_OBJS = \
  $O\7zMethodID.obj \
  $O\7zMethods.obj \

CRYPTO_RAR20_OBJS = \
  $O\Rar20Cipher.obj \
  $O\Rar20Crypto.obj \

CRYPTO_RARAES_OBJS = \
  $O\RarAES.obj \
  $O\sha1.obj \

OBJS = \
  $O\StdAfx.obj \
  $(RAR_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(7Z_OBJS) \
  $(CRYPTO_RAR20_OBJS) \
  $(CRYPTO_RARAES_OBJS) \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(RAR_OBJS): $(*B).cpp
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
$(CRYPTO_RAR20_OBJS): ../../Crypto/Rar20/$(*B).cpp
	$(COMPL)
$(CRYPTO_RARAES_OBJS): ../../Crypto/RarAES/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
