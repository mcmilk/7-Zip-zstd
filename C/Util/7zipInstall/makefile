PROG = 7zipInstall.exe
MY_FIXED = 1

!IFDEF Z7_64BIT_INSTALLER
CFLAGS = $(CFLAGS) -DZ7_64BIT_INSTALLER
!ENDIF

CFLAGS = $(CFLAGS) \
  -DZ7_LZMA_SIZE_OPT \
  -DZ7_NO_METHOD_LZMA2 \
  -DZ7_NO_METHODS_FILTERS \
  -DZ7_USE_NATIVE_BRANCH_FILTER \
  -DZ7_EXTRACT_ONLY \

MAIN_OBJS = \
  $O\7zipInstall.obj \

C_OBJS = \
  $O\7zAlloc.obj \
  $O\7zArcIn.obj \
  $O\7zBuf.obj \
  $O\7zFile.obj \
  $O\7zDec.obj \
  $O\7zStream.obj \
  $O\Bcj2.obj \
  $O\Bra.obj \
  $O\CpuArch.obj \
  $O\DllSecur.obj \
  $O\LzmaDec.obj \

OBJS = \
  $(MAIN_OBJS) \
  $(C_OBJS) \
  $(ASM_OBJS) \
  $O\resource.res

!include "../../../CPP/7zip/Crc.mak"
# !include "../../../CPP/7zip/LzmaDec.mak"

!include "../../../CPP/Build.mak"

$(MAIN_OBJS): $(*B).c
	$(COMPL_O1)
$(C_OBJS): ../../$(*B).c
	$(COMPL_O1)

!include "../../Asm_c.mak"
