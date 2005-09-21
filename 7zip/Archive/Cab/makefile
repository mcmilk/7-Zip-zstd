PROG = cab.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

CAB_OBJS = \
  $O\DllExports.obj \
  $O\CabBlockInStream.obj \
  $O\CabHandler.obj \
  $O\CabHeader.obj \
  $O\CabIn.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\IntToString.obj \
  $O\NewHandler.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\UTFConvert.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\PropVariant.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\LSBFDecoder.obj \
  $O\OutBuffer.obj \
  $O\ProgressUtils.obj \
  $O\StreamUtils.obj \

COMPRESS_LZX_OBJS = \
  $O\LzxDecoder.obj \
  $O\Lzx86Converter.obj \

OBJS = \
  $O\StdAfx.obj \
  $(CAB_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(COMPRESS_LZX_OBJS) \
  $O\DeflateDecoder.obj \
  $O\QuantumDecoder.obj \
  $O\LZOutWindow.obj \
  $O\CopyCoder.obj \
  $O\resource.res

!include "../../../Build.mak"

$(CAB_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(COMPRESS_LZX_OBJS): ../../Compress/Lzx/$(*B).cpp
	$(COMPL_O2)
$O\DeflateDecoder.obj: ../../Compress/Deflate/$(*B).cpp
	$(COMPL_O2)
$O\QuantumDecoder.obj: ../../Compress/Quantum/$(*B).cpp
	$(COMPL)
$O\LZOutWindow.obj: ../../Compress/LZ/$(*B).cpp
	$(COMPL)
$O\CopyCoder.obj: ../../Compress/Copy/$(*B).cpp
	$(COMPL)

