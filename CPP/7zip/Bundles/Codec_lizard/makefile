PROG = lizard.dll
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
  $O\LizardDecoder.obj \
  $O\LizardEncoder.obj \
  $O\LizardRegister.obj \

LIZARD_OBJS = \
  $O/liz_entropy_common.obj \
  $O/liz_fse_compress.obj \
  $O/liz_fse_decompress.obj \
  $O/liz_huf_compress.obj \
  $O/liz_huf_decompress.obj \
  $O/liz_lz5_compress.obj \
  $O/liz_lz5_decompress.obj \
  $O/liz_lz5frame.obj \

ZSTD_OBJS = \
  $O\xxhash.obj \

ZSTDMT_OBJS = \
  $O\threading.obj \
  $O\lizard-mt_common.obj \
  $O\lizard-mt_compress.obj \
  $O\lizard-mt_decompress.obj \

!include "../../7zip.mak"
