PROG = gz.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

GZ_OBJS = \
  $O\GZipHandler.obj \
  $O\GZipHandlerOut.obj \
  $O\GZipHeader.obj \
  $O\GZipIn.obj \
  $O\GZipOut.obj \
  $O\GZipUpdate.obj \
  $O\DllExports.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\PropVariant.obj \

7ZIP_COMMON_OBJS = \
  $O\ProgressUtils.obj \
  $O\StreamUtils.obj \

AR_COMMON_OBJS = \
  $O\CodecsPath.obj \
  $O\InStreamWithCRC.obj \
  $O\OutStreamWithCRC.obj \

OBJS = \
  $O\StdAfx.obj \
  $(GZ_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(GZ_OBJS): $(*B).cpp
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
