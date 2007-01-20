PROG = iso.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

TAR_OBJS = \
  $O\DllExports.obj \
  $O\IsoHandler.obj \
  $O\IsoHeader.obj \
  $O\IsoIn.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\PropVariant.obj \

7ZIP_COMMON_OBJS = \
  $O\LimitedStreams.obj \
  $O\ProgressUtils.obj \
  $O\StreamUtils.obj \

AR_COMMON_OBJS = \
  $O\ItemNameUtils.obj \

OBJS = \
  $O\StdAfx.obj \
  $(TAR_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(COMPRESS_TAR_OBJS) \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(TAR_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)
