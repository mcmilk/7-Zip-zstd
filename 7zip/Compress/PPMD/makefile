PROG = PPMd.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

PPMD_OBJS = \
  $O\DllExports.obj \

PPMD_OPT_OBJS = \
  $O\PPMDDecoder.obj \
  $O\PPMDEncoder.obj \

COMMON_OBJS = \
  $O\Alloc.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\StreamUtils.obj \


OBJS = \
  $O\StdAfx.obj \
  $(PPMD_OBJS) \
  $(PPMD_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $O\resource.res


!include "../../../Build.mak"

$(PPMD_OBJS): $(*B).cpp
	$(COMPL)
$(PPMD_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)

