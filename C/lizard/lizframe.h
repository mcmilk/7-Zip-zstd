/*
   LZ5 auto-framing library
   Header File
   Copyright (C) 2011-2015, Yann Collet.
   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - LZ5 source repository : https://github.com/inikep/lz5
*/

/* LZ5F is a stand-alone API to create LZ5-compressed frames
 * conformant with specification v1.5.1.
 * All related operations, including memory management, are handled internally by the library.
 * You don't need lz5_compress.h when using lz5frame.h.
 * */

#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

/*-************************************
*  Includes
**************************************/
#include <stddef.h>   /* size_t */


/*-************************************
*  Error management
**************************************/
typedef size_t LIZF_errorCode_t;

unsigned    LIZF_isError(LIZF_errorCode_t code);
const char* LIZF_getErrorName(LIZF_errorCode_t code);   /* return error code string; useful for debugging */


/*-************************************
*  Frame compression types
**************************************/
//#define LIZF_DISABLE_OBSOLETE_ENUMS
#ifndef LIZF_DISABLE_OBSOLETE_ENUMS
#  define LIZF_OBSOLETE_ENUM(x) ,x
#else
#  define LIZF_OBSOLETE_ENUM(x)
#endif

typedef enum {
    LIZF_default=0,
    LIZF_max128KB=1,
    LIZF_max256KB=2,
    LIZF_max1MB=3,
    LIZF_max4MB=4,
    LIZF_max16MB=5,
    LIZF_max64MB=6,
    LIZF_max256MB=7
} LIZF_blockSizeID_t;

typedef enum {
    LIZF_blockLinked=0,
    LIZF_blockIndependent
    LIZF_OBSOLETE_ENUM(blockLinked = LIZF_blockLinked)
    LIZF_OBSOLETE_ENUM(blockIndependent = LIZF_blockIndependent)
} LIZF_blockMode_t;

typedef enum {
    LIZF_noContentChecksum=0,
    LIZF_contentChecksumEnabled
    LIZF_OBSOLETE_ENUM(noContentChecksum = LIZF_noContentChecksum)
    LIZF_OBSOLETE_ENUM(contentChecksumEnabled = LIZF_contentChecksumEnabled)
} LIZF_contentChecksum_t;

typedef enum {
    LIZF_frame=0,
    LIZF_skippableFrame
    LIZF_OBSOLETE_ENUM(skippableFrame = LIZF_skippableFrame)
} LIZF_frameType_t;

#ifndef LIZF_DISABLE_OBSOLETE_ENUMS
typedef LIZF_blockSizeID_t blockSizeID_t;
typedef LIZF_blockMode_t blockMode_t;
typedef LIZF_frameType_t frameType_t;
typedef LIZF_contentChecksum_t contentChecksum_t;
#endif

typedef struct {
  LIZF_blockSizeID_t     blockSizeID;           /* max64KB, max256KB, max1MB, max4MB ; 0 == default */
  LIZF_blockMode_t       blockMode;             /* blockLinked, blockIndependent ; 0 == default */
  LIZF_contentChecksum_t contentChecksumFlag;   /* noContentChecksum, contentChecksumEnabled ; 0 == default  */
  LIZF_frameType_t       frameType;             /* LIZF_frame, skippableFrame ; 0 == default */
  unsigned long long     contentSize;           /* Size of uncompressed (original) content ; 0 == unknown */
  unsigned               reserved[2];           /* must be zero for forward compatibility */
} LIZF_frameInfo_t;

typedef struct {
  LIZF_frameInfo_t frameInfo;
  int      compressionLevel;       /* 0 == default (fast mode); values above 16 count as 16; values below 0 count as 0 */
  unsigned autoFlush;              /* 1 == always flush (reduce need for tmp buffer) */
  unsigned reserved[4];            /* must be zero for forward compatibility */
} LIZF_preferences_t;


/*-*********************************
*  Simple compression function
***********************************/
size_t LIZF_compressFrameBound(size_t srcSize, const LIZF_preferences_t* preferencesPtr);

/*!LIZF_compressFrame() :
 * Compress an entire srcBuffer into a valid LZ5 frame, as defined by specification v1.5.1
 * The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
 * You can get the minimum value of dstMaxSize by using LIZF_compressFrameBound()
 * If this condition is not respected, LIZF_compressFrame() will fail (result is an errorCode)
 * The LIZF_preferences_t structure is optional : you can provide NULL as argument. All preferences will be set to default.
 * The result of the function is the number of bytes written into dstBuffer.
 * The function outputs an error code if it fails (can be tested using LIZF_isError())
 */
size_t LIZF_compressFrame(void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LIZF_preferences_t* preferencesPtr);



/*-***********************************
*  Advanced compression functions
*************************************/
typedef struct LIZF_cctx_s* LIZF_compressionContext_t;   /* must be aligned on 8-bytes */

typedef struct {
  unsigned stableSrc;    /* 1 == src content will remain available on future calls to LIZF_compress(); avoid saving src content within tmp buffer as future dictionary */
  unsigned reserved[3];
} LIZF_compressOptions_t;

/* Resource Management */

#define LIZF_VERSION 100
LIZF_errorCode_t LIZF_createCompressionContext(LIZF_compressionContext_t* cctxPtr, unsigned version);
LIZF_errorCode_t LIZF_freeCompressionContext(LIZF_compressionContext_t cctx);
/* LIZF_createCompressionContext() :
 * The first thing to do is to create a compressionContext object, which will be used in all compression operations.
 * This is achieved using LIZF_createCompressionContext(), which takes as argument a version and an LIZF_preferences_t structure.
 * The version provided MUST be LIZF_VERSION. It is intended to track potential version differences between different binaries.
 * The function will provide a pointer to a fully allocated LIZF_compressionContext_t object.
 * If the result LIZF_errorCode_t is not zero, there was an error during context creation.
 * Object can release its memory using LIZF_freeCompressionContext();
 */


/* Compression */

size_t LIZF_compressBegin(LIZF_compressionContext_t cctx, void* dstBuffer, size_t dstMaxSize, const LIZF_preferences_t* prefsPtr);
/* LIZF_compressBegin() :
 * will write the frame header into dstBuffer.
 * dstBuffer must be large enough to accommodate a header (dstMaxSize). Maximum header size is 15 bytes.
 * The LIZF_preferences_t structure is optional : you can provide NULL as argument, all preferences will then be set to default.
 * The result of the function is the number of bytes written into dstBuffer for the header
 * or an error code (can be tested using LIZF_isError())
 */

size_t LIZF_compressBound(size_t srcSize, const LIZF_preferences_t* prefsPtr);
/* LIZF_compressBound() :
 * Provides the minimum size of Dst buffer given srcSize to handle worst case situations.
 * Different preferences can produce different results.
 * prefsPtr is optional : you can provide NULL as argument, all preferences will then be set to cover worst case.
 * This function includes frame termination cost (4 bytes, or 8 if frame checksum is enabled)
 */

size_t LIZF_compressUpdate(LIZF_compressionContext_t cctx, void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LIZF_compressOptions_t* cOptPtr);
/* LIZF_compressUpdate()
 * LIZF_compressUpdate() can be called repetitively to compress as much data as necessary.
 * The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
 * You can get the minimum value of dstMaxSize by using LIZF_compressBound().
 * If this condition is not respected, LIZF_compress() will fail (result is an errorCode).
 * LIZF_compressUpdate() doesn't guarantee error recovery, so you have to reset compression context when an error occurs.
 * The LIZF_compressOptions_t structure is optional : you can provide NULL as argument.
 * The result of the function is the number of bytes written into dstBuffer : it can be zero, meaning input data was just buffered.
 * The function outputs an error code if it fails (can be tested using LIZF_isError())
 */

size_t LIZF_flush(LIZF_compressionContext_t cctx, void* dstBuffer, size_t dstMaxSize, const LIZF_compressOptions_t* cOptPtr);
/* LIZF_flush()
 * Should you need to generate compressed data immediately, without waiting for the current block to be filled,
 * you can call LIZ_flush(), which will immediately compress any remaining data buffered within cctx.
 * Note that dstMaxSize must be large enough to ensure the operation will be successful.
 * LIZF_compressOptions_t structure is optional : you can provide NULL as argument.
 * The result of the function is the number of bytes written into dstBuffer
 * (it can be zero, this means there was no data left within cctx)
 * The function outputs an error code if it fails (can be tested using LIZF_isError())
 */

size_t LIZF_compressEnd(LIZF_compressionContext_t cctx, void* dstBuffer, size_t dstMaxSize, const LIZF_compressOptions_t* cOptPtr);
/* LIZF_compressEnd()
 * When you want to properly finish the compressed frame, just call LIZF_compressEnd().
 * It will flush whatever data remained within compressionContext (like LIZ_flush())
 * but also properly finalize the frame, with an endMark and a checksum.
 * The result of the function is the number of bytes written into dstBuffer (necessarily >= 4 (endMark), or 8 if optional frame checksum is enabled)
 * The function outputs an error code if it fails (can be tested using LIZF_isError())
 * The LIZF_compressOptions_t structure is optional : you can provide NULL as argument.
 * A successful call to LIZF_compressEnd() makes cctx available again for next compression task.
 */


/*-*********************************
*  Decompression functions
***********************************/

typedef struct LIZF_dctx_s* LIZF_decompressionContext_t;   /* must be aligned on 8-bytes */

typedef struct {
  unsigned stableDst;       /* guarantee that decompressed data will still be there on next function calls (avoid storage into tmp buffers) */
  unsigned reserved[3];
} LIZF_decompressOptions_t;


/* Resource management */

/*!LIZF_createDecompressionContext() :
 * Create an LIZF_decompressionContext_t object, which will be used to track all decompression operations.
 * The version provided MUST be LIZF_VERSION. It is intended to track potential breaking differences between different versions.
 * The function will provide a pointer to a fully allocated and initialized LIZF_decompressionContext_t object.
 * The result is an errorCode, which can be tested using LIZF_isError().
 * dctx memory can be released using LIZF_freeDecompressionContext();
 * The result of LIZF_freeDecompressionContext() is indicative of the current state of decompressionContext when being released.
 * That is, it should be == 0 if decompression has been completed fully and correctly.
 */
LIZF_errorCode_t LIZF_createDecompressionContext(LIZF_decompressionContext_t* dctxPtr, unsigned version);
LIZF_errorCode_t LIZF_freeDecompressionContext(LIZF_decompressionContext_t dctx);


/*======   Decompression   ======*/

/*!LIZF_getFrameInfo() :
 * This function decodes frame header information (such as max blockSize, frame checksum, etc.).
 * Its usage is optional. The objective is to extract frame header information, typically for allocation purposes.
 * A header size is variable and can be from 7 to 15 bytes. It's also possible to input more bytes than that.
 * The number of bytes read from srcBuffer will be updated within *srcSizePtr (necessarily <= original value).
 * (note that LIZF_getFrameInfo() can also be used anytime *after* starting decompression, in this case 0 input byte is enough)
 * Frame header info is *copied into* an already allocated LIZF_frameInfo_t structure.
 * The function result is an hint about how many srcSize bytes LIZF_decompress() expects for next call,
 *                        or an error code which can be tested using LIZF_isError()
 *                        (typically, when there is not enough src bytes to fully decode the frame header)
 * Decompression is expected to resume from where it stopped (srcBuffer + *srcSizePtr)
 */
size_t LIZF_getFrameInfo(LIZF_decompressionContext_t dctx,
                         LIZF_frameInfo_t* frameInfoPtr,
                         const void* srcBuffer, size_t* srcSizePtr);

/*!LIZF_decompress() :
 * Call this function repetitively to regenerate data compressed within srcBuffer.
 * The function will attempt to decode *srcSizePtr bytes from srcBuffer, into dstBuffer of maximum size *dstSizePtr.
 *
 * The number of bytes regenerated into dstBuffer will be provided within *dstSizePtr (necessarily <= original value).
 *
 * The number of bytes read from srcBuffer will be provided within *srcSizePtr (necessarily <= original value).
 * If number of bytes read is < number of bytes provided, then decompression operation is not completed.
 * It typically happens when dstBuffer is not large enough to contain all decoded data.
 * LIZF_decompress() must be called again, starting from where it stopped (srcBuffer + *srcSizePtr)
 * The function will check this condition, and refuse to continue if it is not respected.
 *
 * `dstBuffer` is expected to be flushed between each call to the function, its content will be overwritten.
 * `dst` arguments can be changed at will at each consecutive call to the function.
 *
 * The function result is an hint of how many `srcSize` bytes LIZF_decompress() expects for next call.
 * Schematically, it's the size of the current (or remaining) compressed block + header of next block.
 * Respecting the hint provides some boost to performance, since it does skip intermediate buffers.
 * This is just a hint though, it's always possible to provide any srcSize.
 * When a frame is fully decoded, the function result will be 0 (no more data expected).
 * If decompression failed, function result is an error code, which can be tested using LIZF_isError().
 *
 * After a frame is fully decoded, dctx can be used again to decompress another frame.
 */
size_t LIZF_decompress(LIZF_decompressionContext_t dctx,
                       void* dstBuffer, size_t* dstSizePtr,
                       const void* srcBuffer, size_t* srcSizePtr,
                       const LIZF_decompressOptions_t* dOptPtr);



#if defined (__cplusplus)
}
#endif
