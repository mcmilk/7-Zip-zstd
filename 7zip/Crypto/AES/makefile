PROG = AES.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib

AES_OBJS = \
  $O\DllExports.obj \

AES_OPT_OBJS = \
  $O\MyAES.obj \

AES_ORIG_OBJS = \
  $O\aescrypt.obj \
  $O\aeskey.obj \
  $O\aestab.obj \

OBJS = \
  $O\StdAfx.obj \
  $(AES_OBJS) \
  $(AES_OPT_OBJS) \
  $(AES_ORIG_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(AES_OBJS): $(*B).cpp
	$(COMPL)
$(AES_OPT_OBJS): $(*B).cpp
	$(COMPL_O2)
$(AES_ORIG_OBJS): $(*B).c
	$(COMPL_O2)
