PROG = 7z.dll
DEF_FILE = ../../Archive/Archive2.def
CFLAGS = $(CFLAGS) \
  -DEXTERNAL_CODECS \

!IFNDEF UNDER_CE
CFLAGS = $(CFLAGS) -D_7ZIP_LARGE_PAGES
!ENDIF

!include "Arc.mak"

COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\CodecExports.obj \
  $O\ZstdDecoder.obj \
  $O\ZstdEncoder.obj \
  $O\ZstdRegister.obj \

AR_OBJS = $(AR_OBJS) \
  $O\ArchiveExports.obj \
  $O\DllExports2.obj \

ZSTD_OBJS = \
  $O\entropy_common.obj \
  $O\fse_compress.obj \
  $O\fse_decompress.obj \
  $O\huf_compress.obj \
  $O\huf_decompress.obj \
  $O\zbuff_compress.obj \
  $O\zbuff_decompress.obj \
  $O\zstd_common.obj \
  $O\zstd_compress.obj \
  $O\zstd_decompress.obj \
  $O\zstd_v05.obj \
  $O\zstd_v06.obj \
  $O\xxhash.obj \

!include "../../7zip.mak"
