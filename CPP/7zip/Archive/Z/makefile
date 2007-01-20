PROG = z.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

Z_OBJS = \
  $O\DllExports.obj \
  $O\ZHandler.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\NewHandler.obj \

WIN_OBJS = \
  $O\PropVariant.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamUtils.obj \

AR_COMMON_OBJS = \
  $O\DummyOutStream.obj \

COMPRESS_Z_OBJS = \
  $O\ZDecoder.obj \

OBJS = \
  $O\StdAfx.obj \
  $(Z_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(AR_COMMON_OBJS) \
  $(COMPRESS_Z_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(Z_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(AR_COMMON_OBJS): ../Common/$(*B).cpp
	$(COMPL)
$(COMPRESS_Z_OBJS): ../../Compress/Z/$(*B).cpp
	$(COMPL)
