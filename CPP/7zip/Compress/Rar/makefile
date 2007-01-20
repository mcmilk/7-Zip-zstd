PROG = Rar29.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

RAR29_OBJS = \
  $O\DllExports.obj \

RAR29_OPT_OBJS = \
  $O\Rar1Decoder.obj \
  $O\Rar2Decoder.obj \
  $O\Rar3Decoder.obj \
  $O\Rar3Vm.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \
  $O\Vector.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\StreamUtils.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \


OBJS = \
  $O\StdAfx.obj \
  $(RAR29_OBJS) \
  $(RAR29_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(LZ_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(RAR29_OBJS): $(*B).cpp
	$(COMPL)
$(RAR29_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../LZ/$(*B).cpp
	$(COMPL)
