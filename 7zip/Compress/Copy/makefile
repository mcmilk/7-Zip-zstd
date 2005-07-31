PROG = Copy.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

COPY_OBJS = \
  $O\DllExports.obj \
  $O\CopyCoder.obj \

COMMON_OBJS = \
  $O\Alloc.obj \

OBJS = \
  $O\StdAfx.obj \
  $(COPY_OBJS) \
  $(COMMON_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(COPY_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
