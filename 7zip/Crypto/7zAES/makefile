PROG = 7zAES.dll
DEF_FILE = ../Codec.def
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

7ZAES_OBJS = \
  $O\DllExports.obj \

7ZAES_OPT_OBJS = \
  $O\7zAES.obj \
  $O\SHA256.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\NewHandler.obj \
  $O\StringConvert.obj \
  $O\Vector.obj \

WIN_OBJS = \
  $O\DLL.obj \
  $O\Synchronization.obj

7ZIP_COMMON_OBJS = \
  $O\StreamObjects.obj \

OBJS = \
  $O\StdAfx.obj \
  $(7ZAES_OBJS) \
  $(7ZAES_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(WIN_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $O\resource.res

!include "../../../Build.mak"

$(7ZAES_OBJS): $(*B).cpp
	$(COMPL)
$(7ZAES_OPT_OBJS): $(*B).cpp
	$(COMPL)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(WIN_OBJS): ../../../Windows/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
