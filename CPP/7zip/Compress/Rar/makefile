PROG = Rar29.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExports.obj \

RAR29_OPT_OBJS = \
  $O\Rar1Decoder.obj \
  $O\Rar2Decoder.obj \
  $O\Rar3Decoder.obj \
  $O\Rar3Vm.obj \
  $O\RarCodecsRegister.obj \

COMMON_OBJS = \
  $O\CRC.obj \
  $O\Vector.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\StreamUtils.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

C_OBJS = \
  $O\Alloc.obj \

!include "../../Crc2.mak"

OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(RAR29_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(LZ_OBJS) \
  $(C_OBJS) \
  $(CRC_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(COMPRESS_OBJS): ../$(*B).cpp
	$(COMPL)
$(RAR29_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../LZ/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)

!include "../../Crc.mak"
