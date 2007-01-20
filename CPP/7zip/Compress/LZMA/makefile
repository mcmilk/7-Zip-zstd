PROG = LZMA.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../ -DCOMPRESS_MF_MT -D_ST_MODE
LIBS = $(LIBS) oleaut32.lib

LZMA_OBJS = \
  $O\DllExports.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMAEncoder.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \

WIN_OBJS = \
  $O\Synchronization.obj

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\StreamUtils.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

C_OBJS = \
  $O\7zCrc.obj \
  $O\Sort.obj \
  $O\Threads.obj \

C_LZ_OBJS = \
  $O\MatchFinder.obj \
  $O\MatchFinderMt.obj \

OBJS = \
  $O\StdAfx.obj \
  $(LZMA_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(LZ_OBJS) \
  $(C_OBJS) \
  $(C_LZ_OBJS) \
  $O\RangeCoderBit.obj \
  $O\resource.res

!include "../../../Build.mak"

$(LZMA_OBJS): $(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
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
