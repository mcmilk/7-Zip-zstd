PROG = Deflate.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../ -D_ST_MODE
LIBS = $(LIBS) oleaut32.lib

DEFLATE_OBJS = \
  $O\DllExports.obj \

DEFLATE_OPT_OBJS = \
  $O\DeflateDecoder.obj \
  $O\DeflateEncoder.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\LSBFDecoder.obj \
  $O\LSBFEncoder.obj \

LZ_OBJS = \
  $O\LZInWindow.obj \
  $O\LZOutWindow.obj \


OBJS = \
  $O\StdAfx.obj \
  $(DEFLATE_OBJS) \
  $(DEFLATE_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(LZ_OBJS) \
  $O\HuffmanEncoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(DEFLATE_OBJS): $(*B).cpp
	$(COMPL)
$(DEFLATE_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../LZ/$(*B).cpp
	$(COMPL)
$O\HuffmanEncoder.obj: ../Huffman/$(*B).cpp
	$(COMPL_O2)
