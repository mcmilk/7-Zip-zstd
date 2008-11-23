MY_STATIC_LINK=1

PROG = 7zDec.exe

C_OBJS = \
  $O\7zBuf.obj \
  $O\7zBuf2.obj \
  $O\7zCrc.obj \
  $O\LzmaDec.obj \
  $O\Bra86.obj \
  $O\Bcj2.obj \
  $O\7zFile.obj \
  $O\7zStream.obj \

7Z_OBJS = \
  $O\7zAlloc.obj \
  $O\7zDecode.obj \
  $O\7zExtract.obj \
  $O\7zHeader.obj \
  $O\7zIn.obj \
  $O\7zItem.obj \
  $O\7zMain.obj \

OBJS = \
  $(7Z_OBJS) \
  $(C_OBJS) \

!include "../../../CPP/Build.mak"

$(7Z_OBJS): $(*B).c
	$(COMPL_O1)
$(C_OBJS): ../../$(*B).c
	$(COMPL_O2)
