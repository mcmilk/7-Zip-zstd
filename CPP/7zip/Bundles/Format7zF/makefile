PROG = 7z.dll
# USE_C_LZFINDOPT = 1
DEF_FILE = ../../Archive/Archive2.def
CFLAGS = $(CFLAGS) \
  -DZ7_EXTERNAL_CODECS \

!IFNDEF UNDER_CE
# CFLAGS = $(CFLAGS) -DZ7_LARGE_PAGES
!ENDIF

!include "Arc.mak"

COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\CodecExports.obj \

AR_OBJS = $(AR_OBJS) \
  $O\ArchiveExports.obj \
  $O\DllExports2.obj \


!include "../../7zip.mak"
