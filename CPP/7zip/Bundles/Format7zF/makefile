PROG = 7z.dll
DEF_FILE = ../../Archive/Archive2.def
CFLAGS = $(CFLAGS) \
  -DEXTERNAL_CODECS -DZSTD_LEGACY \

!IFNDEF UNDER_CE
CFLAGS = $(CFLAGS) -D_7ZIP_LARGE_PAGES
!ENDIF

!include "Arc.mak"

COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\CodecExports.obj \

AR_OBJS = $(AR_OBJS) \
  $O\ArchiveExports.obj \
  $O\DllExports2.obj \


!include "../../zstd.mak"
ZSTD_OBJS = $(ZSTD_OBJS) \
  $O\fse_compress.obj \
  $O\huf_compress.obj \
  $O\zbuff_compress.obj \
  $O\zstd_compress.obj \
  $O\zstd_v07.obj \
  $O\zstd_v06.obj \
  $O\zstd_v05.obj \

!include "../../7zip.mak"
