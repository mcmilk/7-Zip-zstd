PROG = Swap.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExports.obj \

SWAP_OPT_OBJS = \
  $O\ByteSwap.obj \
  $O\ByteSwapRegister.obj \

OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(SWAP_OPT_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(COMPRESS_OBJS): ../$(*B).cpp
	$(COMPL)
$(SWAP_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
