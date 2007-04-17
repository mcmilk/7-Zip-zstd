PROG = Deflate.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExports.obj \

DEFLATE_OPT_OBJS = \
  $O\DeflateDecoder.obj \
  $O\DeflateEncoder.obj \
  $O\DeflateRegister.obj \
  $O\Deflate64Register.obj \
  $O\DeflateNsisRegister.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\LSBFDecoder.obj \
  $O\LSBFEncoder.obj \

LZ_OBJS = \
  $O\LZOutWindow.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\7zCrc.obj \
  $O\Sort.obj \

C_LZ_OBJS = \
  $O\MatchFinder.obj \

!include "../../Crc2.mak"

OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(DEFLATE_OPT_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(LZ_OBJS) \
  $(C_OBJS) \
  $(C_LZ_OBJS) \
  $O\HuffmanEncode.obj \
  $O\resource.res

!include "../../../Build.mak"

$(COMPRESS_OBJS): ../$(*B).cpp
	$(COMPL)
$(DEFLATE_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../LZ/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
$(C_LZ_OBJS): ../../../../C/Compress/Lz/$(*B).c
	$(COMPL_O2)
$O\HuffmanEncode.obj: ../../../../C/Compress/Huffman/$(*B).c
	$(COMPL_O2)
!include "../../Crc.mak"
