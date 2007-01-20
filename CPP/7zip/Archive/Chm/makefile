PROG = chm.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

CHM_OBJS = \
  $O\DllExports.obj \
  $O\ChmHandler.obj \
  $O\ChmHeader.obj \
  $O\ChmIn.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\UTFConvert.obj \
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

COMPRESS_LZX_OBJS = \
  $O\LzxDecoder.obj \
  $O\Lzx86Converter.obj \

OBJS = \
  $O\StdAfx.obj \
  $(CHM_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(COMPRESS_LZX_OBJS) \
  $O\LZOutWindow.obj \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(CHM_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(COMPRESS_LZX_OBJS): ../../Compress/Lzx/$(*B).cpp
	$(COMPL_O2)
$O\LZOutWindow.obj: ../../Compress/LZ/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)

