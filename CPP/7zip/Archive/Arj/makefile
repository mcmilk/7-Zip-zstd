PROG = arj.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

ARJ_OBJS = \
  $O\DllExports.obj \
  $O\ArjHandler.obj \
  $O\ArjIn.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\PropVariant.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\LimitedStreams.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamUtils.obj \

AR_COMMON_OBJS = \
  $O\ItemNameUtils.obj \
  $O\OutStreamWithCRC.obj \

COMPRESS_ARJ_OBJS = \
  $O\ArjDecoder1.obj \
  $O\ArjDecoder2.obj \

OBJS = \
  $O\StdAfx.obj \
  $(ARJ_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(COMPRESS_ARJ_OBJS) \
  $O\CopyCoder.obj \
  $O\LZOutWindow.obj \
  $O\resource.res

!include "../../../Build.mak"

$(ARJ_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(COMPRESS_ARJ_OBJS): ../../Compress/Arj/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$O\LZOutWindow.obj: ../../Compress/LZ/$(*B).cpp
	$(COMPL)
