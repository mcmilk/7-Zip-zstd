PROG = zstd.dll
DEF_FILE = ../../Compress/Codec.def
CFLAGS = $(CFLAGS)

#EXPORTS
#  CreateObject PRIVATE
#  GetNumberOfMethods PRIVATE
#  GetMethodProperty PRIVATE

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
  $O\zbuff_compress.obj \
  $O\zstd_compress.obj \

!include "../../7zip.mak"
