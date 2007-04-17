PROG = Branch.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

COMPRESS_OBJS = \
  $O\CodecExports.obj \
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
  $O\BranchRegister.obj \
  $O\BCJRegister.obj \
  $O\BCJ2Register.obj \

C_OBJS = \
  $O\Alloc.obj \

C_BRANCH_OBJS = \
  $O\BranchARM.obj \
  $O\BranchARMThumb.obj \
  $O\BranchIA64.obj \
  $O\BranchPPC.obj \
  $O\BranchSPARC.obj \
  $O\BranchX86.obj \

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \


OBJS = \
  $O\StdAfx.obj \
  $(COMPRESS_OBJS) \
  $(BRANCH_OPT_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $O\RangeCoderBit.obj \
  $(C_OBJS) \
  $(C_BRANCH_OBJS) \
  $O\resource.res


!include "../../../Build.mak"

$(COMPRESS_OBJS): ../$(*B).cpp
	$(COMPL)
$(BRANCH_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$O\RangeCoderBit.obj: ../RangeCoder/$(*B).cpp
	$(COMPL)
$(C_OBJS): ../../../../C/$(*B).c
	$(COMPL_O2)
$(C_BRANCH_OBJS): ../../../../C/Compress/Branch/$(*B).c
	$(COMPL_O2)
