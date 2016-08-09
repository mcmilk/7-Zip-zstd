
COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\ZstdDecoder.obj \
  $O\ZstdEncoder.obj \
  $O\ZstdRegister.obj \

ZSTD_OBJS = \
  $O\entropy_common.obj \
  $O\fse_decompress.obj \
  $O\huf_decompress.obj \
  $O\zbuff_decompress.obj \
  $O\zstd_common.obj \
  $O\zstd_decompress.obj \
  $O\xxhash.obj \
