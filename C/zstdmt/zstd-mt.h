
/**
 * Copyright (c) 2016 Tino Reichardt
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * You can contact the author at:
 * - zstdmt source repository: https://github.com/mcmilk/zstdmt
 */

/* ***************************************
 * Defines
 ****************************************/

#ifndef ZSTDCB_H
#define ZSTDCB_H

#if defined (__cplusplus)
extern "C" {
#endif

#include <stddef.h>   /* size_t */

#define ZSTDCB_THREAD_MAX 128
#define ZSTDCB_LEVEL_MIN    1
#define ZSTDCB_LEVEL_MAX   22

/* zstd magic values */
#define ZSTDCB_MAGICNUMBER_V01  0x1EB52FFDU
#define ZSTDCB_MAGICNUMBER_MIN  0xFD2FB522U
#define ZSTDCB_MAGICNUMBER_MAX  0xFD2FB528U
#define ZSTDCB_MAGIC_SKIPPABLE  0x184D2A50U

/* **************************************
 * Error Handling
 ****************************************/

typedef enum {
  ZSTDCB_error_no_error,
  ZSTDCB_error_memory_allocation,
  ZSTDCB_error_init_missing,
  ZSTDCB_error_read_fail,
  ZSTDCB_error_write_fail,
  ZSTDCB_error_data_error,
  ZSTDCB_error_frame_compress,
  ZSTDCB_error_frame_decompress,
  ZSTDCB_error_compressionParameter_unsupported,
  ZSTDCB_error_compression_library,
  ZSTDCB_error_canceled,
  ZSTDCB_error_maxCode
} ZSTDCB_ErrorCode;

extern size_t zstdmt_errcode;

#define ZSTDCB_PREFIX(name) ZSTDCB_error_##name
#define ZSTDCB_ERROR(name) ((size_t)-ZSTDCB_PREFIX(name))
extern unsigned ZSTDCB_isError(size_t code);
extern const char* ZSTDCB_getErrorString(size_t code);

/* **************************************
 * Structures
 ****************************************/

typedef struct {
	void *buf;		/* ptr to data */
	size_t size;		/* current filled in buf */
	size_t allocated;	/* length of buf */
} ZSTDCB_Buffer;

/**
 * reading and writing functions
 * - you can use stdio functions or plain read/write
 * - just write some wrapper on your own
 * - a sample is given in 7-Zip ZS
 *
 * error definitions:
 *  0 = success
 * -1 = generic read/write error
 * -2 = user abort
 * -3 = memory
 */
typedef int (fn_read) (void *args, ZSTDCB_Buffer * in);
typedef int (fn_write) (void *args, ZSTDCB_Buffer * out);

typedef struct {
	fn_read *fn_read;
	void *arg_read;
	fn_write *fn_write;
	void *arg_write;
} ZSTDCB_RdWr_t;

/* **************************************
 * Compression
 ****************************************/

typedef struct ZSTDCB_CCtx_s ZSTDCB_CCtx;

/**
 * ZSTDCB_createCCtx() - allocate new compression context
 *
 * This function allocates and initializes an zstd commpression context.
 * The context can be used multiple times without the need for resetting
 * or re-initializing.
 *
 * @level: compression level, which should be used (1..22)
 * @threads: number of threads, which should be used (1..ZSTDCB_THREAD_MAX)
 * @inputsize: - if zero, becomes some optimal value for the level
 *             - if nonzero, the given value is taken
 * @zstdmt_errcode: space for storing zstd errors (needed for thread safety)
 * @return: the context on success, zero on error
 */
ZSTDCB_CCtx *ZSTDCB_createCCtx(int threads, int level, int inputsize);

/**
 * ZSTDCB_compressDCtx() - threaded compression for zstd
 *
 * This function will create valid zstd streams. The number of threads,
 * the input chunksize and the compression level are ....
 *
 * @ctx: context, which needs to be created with ZSTDCB_createDCtx()
 * @rdwr: callback structure, which defines reding/writing functions
 * @return: zero on success, or error code
 */
size_t ZSTDCB_compressCCtx(ZSTDCB_CCtx * ctx, ZSTDCB_RdWr_t * rdwr);

/**
 * ZSTDCB_GetFramesCCtx() - number of written frames
 * ZSTDCB_GetInsizeCCtx() - read bytes of input
 * ZSTDCB_GetOutsizeCCtx() - written bytes of output
 *
 * These three functions will return some statistical data of the
 * compression context ctx.
 *
 * @ctx: context, which should be examined
 * @return: the request value, or zero on error
 */
size_t ZSTDCB_GetFramesCCtx(ZSTDCB_CCtx * ctx);
size_t ZSTDCB_GetInsizeCCtx(ZSTDCB_CCtx * ctx);
size_t ZSTDCB_GetOutsizeCCtx(ZSTDCB_CCtx * ctx);

/**
 * ZSTDCB_freeCCtx() - free compression context
 *
 * This function will free all allocated resources, which were allocated
 * by ZSTDCB_createCCtx(). This function can not fail.
 *
 * @ctx: context, which should be freed
 */
void ZSTDCB_freeCCtx(ZSTDCB_CCtx * ctx);

/* **************************************
 * Decompression
 ****************************************/

typedef struct ZSTDCB_DCtx_s ZSTDCB_DCtx;

/**
 * 1) allocate new cctx
 * - return cctx or zero on error
 *
 * @level   - 1 .. 22
 * @threads - 1 .. ZSTDCB_THREAD_MAX
 * @srclen  - the max size of src for ZSTDCB_compressCCtx()
 * @dstlen  - the min size of dst
 */
ZSTDCB_DCtx *ZSTDCB_createDCtx(int threads, int inputsize);

/**
 * ZSTDCB_decompressDCtx() - threaded decompression for zstd
 *
 * This function will decompress valid zstd streams.
 *
 * @ctx: context, which needs to be created with ZSTDCB_createDCtx()
 * @rdwr: callback structure, which defines reding/writing functions
 * @return: zero on success, or error code
 */
size_t ZSTDCB_decompressDCtx(ZSTDCB_DCtx * ctx, ZSTDCB_RdWr_t * rdwr);

/**
 * ZSTDCB_GetFramesDCtx() - number of read frames
 * ZSTDCB_GetInsizeDCtx() - read bytes of input
 * ZSTDCB_GetOutsizeDCtx() - written bytes of output
 *
 * These three functions will return some statistical data of the
 * decompression context ctx.
 *
 * @ctx: context, which should be examined
 * @return: the request value, or zero on error
 */
size_t ZSTDCB_GetFramesDCtx(ZSTDCB_DCtx * ctx);
size_t ZSTDCB_GetInsizeDCtx(ZSTDCB_DCtx * ctx);
size_t ZSTDCB_GetOutsizeDCtx(ZSTDCB_DCtx * ctx);

/**
 * ZSTDCB_freeDCtx() - free decompression context
 *
 * This function will free all allocated resources, which were allocated
 * by ZSTDCB_createDCtx(). This function can not fail.
 *
 * @ctx: context, which should be freed
 */
void ZSTDCB_freeDCtx(ZSTDCB_DCtx * ctx);

#if defined (__cplusplus)
}
#endif
#endif				/* ZSTDCB_H */
