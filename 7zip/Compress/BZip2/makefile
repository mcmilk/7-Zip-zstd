PROG = BZip2.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

BZIP2_OBJS = \
  $O\DllExports.obj \
  $O\BZip2CRC.obj \

BZIP2_OPT_OBJS = \
  $O\BZip2Decoder.obj \
  $O\BZip2Encoder.obj \

COMMON_OBJS = \
  $O\Alloc.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \

OBJS = \
  $O\StdAfx.obj \
  $(BZIP2_OBJS) \
  $(BZIP2_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $O\BlockSort.obj \
  $O\HuffmanEncoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(BZIP2_OBJS): $(*B).cpp
	$(COMPL)
$(BZIP2_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$O\BlockSort.obj: ../BWT/$(*B).cpp
	$(COMPL_O2)
$O\HuffmanEncoder.obj: ../Huffman/$(*B).cpp
	$(COMPL_O2)
