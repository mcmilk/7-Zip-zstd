PROG = cab.dll
DEF_FILE = ../Archive.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

CAB_OBJS = \
  $O\DllExports.obj \
  $O\CabCopyDecoder.obj \
  $O\CabHandler.obj \
  $O\CabHeader.obj \
  $O\CabIn.obj \
  $O\CabInBuffer.obj \
  $O\LZXDecoder.obj \
  $O\LZXi86Converter.obj \
  $O\MSZipDecoder.obj \

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

OBJS = \
  $O\StdAfx.obj \
  $(CAB_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $O\LZOutWindow.obj \
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
$O\LZOutWindow.obj: ../../Compress/LZ/$(*B).cpp
	$(COMPL)
