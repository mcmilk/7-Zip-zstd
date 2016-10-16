PROG = lz4.dll
DEF_FILE = ../../Compress/Codec.def
CFLAGS = $(CFLAGS) -DNEED_7ZIP_GUID

7ZIP_COMMON_OBJS = \
  $O\StreamUtils.obj \

WIN_OBJS = \
  $O\System.obj \

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExportsCompress.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\Threads.obj \

COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\Lz4Decoder.obj \
  $O\Lz4Encoder.obj \
  $O\Lz4Register.obj \

LZ4_OBJS = \
  $O\lz4.obj \
  $O\lz4frame.obj \
  $O\lz4hc.obj \
  $O\lz4xxhash.obj \

ZSTDMT_OBJS = \
  $O\lz4mt_common.obj \
  $O\lz4mt_compress.obj \
  $O\lz4mt_decompress.obj \
  $O\threading.obj \

!include "../../7zip.mak"
