PROG = Branch.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

BRANCH_OBJS = \
  $O\DllExports.obj \

BRANCH_OPT_OBJS = \
  $O\ARM.obj \
  $O\ARMThumb.obj \
  $O\BranchCoder.obj \
  $O\IA64.obj \
  $O\PPC.obj \
  $O\SPARC.obj \
  $O\x86.obj \
  $O\x86_2.obj \

COMMON_OBJS = \
  $O\Alloc.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \


OBJS = \
  $O\StdAfx.obj \
  $(BRANCH_OBJS) \
  $(BRANCH_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $O\RangeCoderBit.obj \
  $O\resource.res


!include "../../../Build.mak"

$(BRANCH_OBJS): $(*B).cpp
	$(COMPL)
$(BRANCH_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$O\RangeCoderBit.obj: ../RangeCoder/$(*B).cpp
	$(COMPL)
