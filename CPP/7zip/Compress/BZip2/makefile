PROG = BZip2.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../  -DCOMPRESS_BZIP2_MT -D_7ZIP_LARGE_PAGES
LIBS = $(LIBS) oleaut32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExports.obj \

BZIP2_OBJS = \
  $O\BZip2CRC.obj \
  $O\BZip2Register.obj \

BZIP2_OPT_OBJS = \
  $O\BZip2Decoder.obj \
  $O\BZip2Encoder.obj \

WIN_OBJS = \
  $O\Synchronization.obj

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\Sort.obj \

OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(BZIP2_OBJS) \
  $(BZIP2_OPT_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $O\BlockSort.obj \
  $(C_OBJS) \
  $O\HuffmanEncode.obj \
  $O\resource.res

!include "../../../Build.mak"

$(COMPRESS_OBJS): ../$(*B).cpp
	$(COMPL)
$(BZIP2_OBJS): $(*B).cpp
	$(COMPL)
$(BZIP2_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$O\BlockSort.obj: ../BWT/$(*B).cpp
	$(COMPL_O2)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
$O\HuffmanEncode.obj: ../../../../C/Compress/Huffman/$(*B).c
	$(COMPL_O2)
