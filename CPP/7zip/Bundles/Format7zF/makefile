PROG = 7z.dll
DEF_FILE = ../../Archive/Archive2.def
CFLAGS = $(CFLAGS) \
  -DEXTERNAL_CODECS -DZSTD_LEGACY_SUPPORT \

!IFNDEF UNDER_CE
CFLAGS = $(CFLAGS) -D_7ZIP_LARGE_PAGES
!ENDIF

!include "Arc.mak"

COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\CodecExports.obj \

AR_OBJS = $(AR_OBJS) \
  $O\ArchiveExports.obj \
  $O\DllExports2.obj \

COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\Lz4Decoder.obj \
  $O\Lz4Encoder.obj \
  $O\Lz4Register.obj \
  $O\Lz5Decoder.obj \
  $O\Lz5Encoder.obj \
  $O\Lz5Register.obj \
  $O\ZstdDecoder.obj \
  $O\ZstdEncoder.obj \
  $O\ZstdRegister.obj \

LZ4_OBJS = \
  $O\lz4.obj \
  $O\lz4frame.obj \
  $O\lz4hc.obj \
  $O\lz4xxhash.obj \

LZ5_OBJS = \
  $O\lz5.obj \
  $O\lz5frame.obj \
  $O\lz5hc.obj \

ZSTD_OBJS = \
  $O\entropy_common.obj \
  $O\fse_decompress.obj \
  $O\huf_decompress.obj \
  $O\zstd_common.obj \
  $O\zstd_decompress.obj \
  $O\xxhash.obj \
  $O\fse_compress.obj \
  $O\huf_compress.obj \
  $O\zstd_compress.obj \
  $O\error_private.obj \
  $O\zstd_v01.obj \
  $O\zstd_v02.obj \
  $O\zstd_v03.obj \
  $O\zstd_v04.obj \
  $O\zstd_v05.obj \
  $O\zstd_v06.obj \
  $O\zstd_v07.obj \

ZSTDMT_OBJS = \
  $O\lz5mt_common.obj \
  $O\lz5mt_compress.obj \
  $O\lz5mt_decompress.obj \
  $O\lz4mt_common.obj \
  $O\lz4mt_compress.obj \
  $O\lz4mt_decompress.obj \
  $O\threading.obj \
  $O\zstdmt_common.obj \
  $O\zstdmt_compress.obj \
  $O\zstdmt_decompress.obj \

!include "../../7zip.mak"
