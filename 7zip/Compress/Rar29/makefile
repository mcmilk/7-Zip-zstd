PROG = Rar29.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

RAR29_OBJS = \
  $O\DllExports.obj \
  $O\Rar29Decoder.obj \

RAR29_OPT_OBJS = \
  $O\crc.obj \
  $O\errhnd.obj \
  $O\getbits.obj \
  $O\rarvm.obj \
  $O\rdwrfn.obj \
  $O\resource.obj \
  $O\smallfn.obj \
  $O\system.obj \
  $O\unpack.obj \

RAR20_OPT_OBJS = \
  $O\Rar20Decoder.obj \
  $O\Rar20Multimedia.obj \

COMMON_OBJS = \
  $O\Alloc.obj \

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
  $(RAR20_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(LZ_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

COMPL_SPEC = $(CPP) $(CFLAGS_O2) -DSILENT -DNOCRYPT -DNOVOLUME $** 

$(RAR29_OBJS): $(*B).cpp
	$(COMPL)
$(RAR29_OPT_OBJS): Original/$(*B).cpp
	$(COMPL_SPEC)
$(RAR20_OPT_OBJS): ../Rar20/$(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../LZ/$(*B).cpp
	$(COMPL)
