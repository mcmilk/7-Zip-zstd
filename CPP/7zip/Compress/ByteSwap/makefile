PROG = Swap.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

SWAP_OBJS = \
  $O\DllExports.obj \

SWAP_OPT_OBJS = \
  $O\ByteSwap.obj \

OBJS = \
  $O\StdAfx.obj \
  $(SWAP_OBJS) \
  $(SWAP_OPT_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(SWAP_OBJS): $(*B).cpp
	$(COMPL)
$(SWAP_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
