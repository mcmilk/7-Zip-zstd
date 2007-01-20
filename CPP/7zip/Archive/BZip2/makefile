PROG = bz2.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../ -DCOMPRESS_MT
LIBS = $(LIBS) oleaut32.lib user32.lib

BZ2_OBJS = \
  $O\BZip2Handler.obj \
  $O\BZip2HandlerOut.obj \
  $O\BZip2Update.obj \
  $O\DllExports.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
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
  $O\DummyOutStream.obj \
  $O\ParseProperties.obj \

OBJS = \
  $O\StdAfx.obj \
  $(BZ2_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(BZ2_OBJS): $(*B).cpp
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
