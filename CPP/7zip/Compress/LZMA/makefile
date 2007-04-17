PROG = LZMA.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../ \
  -DCOMPRESS_MF_MT \
  -D_7ZIP_LARGE_PAGES \

LIBS = $(LIBS) oleaut32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExports.obj \

COMMON_OBJS = \
  $O\CRC.obj \
  
LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMAEncoder.obj \
  $O\LZMARegister.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\StreamUtils.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

C_OBJS = \
  $O\Threads.obj \
  $O\Alloc.obj \

C_LZ_OBJS = \
  $O\MatchFinder.obj \
  $O\MatchFinderMt.obj \

!include "../../Crc2.mak"

OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(COMMON_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(LZ_OBJS) \
  $(C_OBJS) \
  $(C_LZ_OBJS) \
  $(CRC_OBJS) \
  $O\RangeCoderBit.obj \
  $O\resource.res

!include "../../../Build.mak"

$(COMPRESS_OBJS): ../$(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../LZ/$(*B).cpp
	$(COMPL)
$O\RangeCoderBit.obj: ../RangeCoder/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
$(C_LZ_OBJS): ../../../../C/Compress/Lz/$(*B).c
	$(COMPL_O2)
!include "../../Crc.mak"
