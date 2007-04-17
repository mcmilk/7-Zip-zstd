PROG = PPMd.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../ -D_7ZIP_LARGE_PAGES
LIBS = $(LIBS) oleaut32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExports.obj \

PPMD_OPT_OBJS = \
  $O\PPMDDecoder.obj \
  $O\PPMDEncoder.obj \
  $O\PPMDRegister.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\StreamUtils.obj \

C_OBJS = \
  $O\Alloc.obj \

OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(PPMD_OPT_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(C_OBJS) \
  $O\resource.res


!include "../../../Build.mak"

$(COMPRESS_OBJS): ../$(*B).cpp
	$(COMPL)
$(PPMD_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)


