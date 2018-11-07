PROG = flzma2.dll
DEF_FILE = ../../Compress/Codec.def
CFLAGS = $(CFLAGS) -DNEED_7ZIP_GUID -DNO_XXHASH

7ZIP_COMMON_OBJS = \
  $O\CWrappers.obj \
  $O\StreamUtils.obj \

WIN_OBJS = \
  $O\System.obj \

COMPRESS_OBJS = \
  $O\CodecExports.obj \
  $O\DllExportsCompress.obj \

C_OBJS = \
  $O\Alloc.obj \
  $O\Threads.obj \
  $O\LzFind.obj \
  $O\LzFindMt.obj \
  $O\Lzma2Dec.obj \
  $O\Lzma2DecMt.obj \
  $O\Lzma2Enc.obj \
  $O\LzmaDec.obj \
  $O\LzmaEnc.obj \
  $O\MtCoder.obj \
  $O\MtDec.obj \

!include "../../LzmaDec.mak"

COMPRESS_OBJS = $(COMPRESS_OBJS) \
  $O\Lzma2Decoder.obj \
  $O\Lzma2Encoder.obj \
  $O\LzmaDecoder.obj \
  $O\LzmaEncoder.obj \
  $O\FastLzma2Register.obj \

FASTLZMA2_OBJS = \
  $O\fl2_error_private.obj \
  $O\fl2pool.obj \
  $O\fl2threading.obj \
  $O\fl2_common.obj \
  $O\fl2_compress.obj \
  $O\lzma2_enc.obj \
  $O\radix_bitpack.obj \
  $O\radix_mf.obj \
  $O\radix_struct.obj \
  $O\range_enc.obj \

!include "../../7zip.mak"
