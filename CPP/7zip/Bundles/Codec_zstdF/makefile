PROG = zstd.dll
DEF_FILE = ../../Compress/Codec.def
CFLAGS = $(CFLAGS) -DNEED_7ZIP_GUID -DZSTD_LEGACY_SUPPORT

7ZIP_COMMON_OBJS = \
  $O\StreamUtils.obj \

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExportsCompress.obj \

C_OBJS = \
  $O\Alloc.obj \

!include "../../zstd.mak"
ZSTD_OBJS = $(ZSTD_OBJS) \
  $O\fse_compress.obj \
  $O\huf_compress.obj \
  $O\zstd_compress.obj \
  $O\zstd_v01.obj \
  $O\zstd_v02.obj \
  $O\zstd_v03.obj \
  $O\zstd_v04.obj \
  $O\zstd_v05.obj \
  $O\zstd_v06.obj \
  $O\zstd_v07.obj \

!include "../../7zip.mak"
