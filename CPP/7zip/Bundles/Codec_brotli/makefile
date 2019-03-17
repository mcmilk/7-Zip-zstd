PROG = brotli.dll
DEF_FILE = ../../Compress/Codec.def
CFLAGS = $(CFLAGS)

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
  $O\BrotliDecoder.obj \
  $O\BrotliEncoder.obj \
  $O\BrotliRegister.obj \

BROTLI_OBJS = \
  $O/br_backward_references.obj \
  $O/br_backward_references_hq.obj \
  $O/br_bit_cost.obj \
  $O/br_bit_reader.obj \
  $O/br_block_splitter.obj \
  $O/br_brotli_bit_stream.obj \
  $O/br_cluster.obj \
  $O/br_compress_fragment.obj \
  $O/br_compress_fragment_two_pass.obj \
  $O/br_decode.obj \
  $O/br_dictionary.obj \
  $O/br_dictionary_hash.obj \
  $O/br_encode.obj \
  $O/br_encoder_dict.obj \
  $O/br_entropy_encode.obj \
  $O/br_histogram.obj \
  $O/br_huffman.obj \
  $O/br_literal_cost.obj \
  $O/br_memory.obj \
  $O/br_metablock.obj \
  $O/br_state.obj \
  $O/br_static_dict.obj \
  $O/br_transform.obj \
  $O/br_utf8_util.obj \

ZSTDMT_OBJS = \
  $O\brotli-mt_common.obj \
  $O\brotli-mt_compress.obj \
  $O\brotli-mt_decompress.obj \
  $O\zstd-mt_threading.obj \

!include "../../7zip.mak"
