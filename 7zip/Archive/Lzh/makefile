PROG = lzh.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

LZH_OBJS = \
  $O\DllExports.obj \
  $O\LzhCRC.obj \
  $O\LzhHandler.obj \
  $O\LzhIn.obj \
  $O\LzhOutStreamWithCRC.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\PropVariant.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\LimitedStreams.obj \
  $O\ProgressUtils.obj \

AR_COMMON_OBJS = \
  $O\ItemNameUtils.obj \

COMPRESS_LZH_OBJS = \
  $O\LzhDecoder.obj \

OBJS = \
  $O\StdAfx.obj \
  $(LZH_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(COMPRESS_LZH_OBJS) \
  $O\CopyCoder.obj \
  $O\LZOutWindow.obj \
  $O\resource.res

!include "../../../Build.mak"

$(LZH_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(COMPRESS_LZH_OBJS): ../../Compress/Lzh/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
$O\LZOutWindow.obj: ../../Compress/LZ/$(*B).cpp
	$(COMPL)
