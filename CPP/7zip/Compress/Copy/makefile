PROG = Copy.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExports.obj \

COPY_OBJS = \
  $O\CopyCoder.obj \
  $O\CopyRegister.obj \

7ZIP_COMMON_OBJS = \
  $O\StreamUtils.obj \

C_OBJS = \
  $O\Alloc.obj \

OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(COPY_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(C_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(COMPRESS_OBJS): ../$(*B).cpp
	$(COMPL)
$(COPY_OBJS): $(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
