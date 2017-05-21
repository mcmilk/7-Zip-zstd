/*
   LZ5 - Fast LZ compression algorithm
   Header File
   Copyright (C) 2011-2016, Yann Collet.
   Copyright (C) 2016, Przemyslaw Skibinski <inikep@gmail.com>

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
#ifndef LZ5_H_2983
#define LZ5_H_2983

#if defined (__cplusplus)
extern "C" {
#endif

/*
 * lz5_compress.h provides block compression functions. It gives full buffer control to user.
 * Block compression functions are not-enough to send information,
 * since it's still necessary to provide metadata (such as compressed size),
 * and each application can do it in whichever way it wants.
 * For interoperability, there is LZ5 frame specification (lz5_Frame_format.md).
 * A library is provided to take care of it, see lz5frame.h.
*/


/*^***************************************************************
*  Export parameters
*****************************************************************/
/*
*  LZ5_DLL_EXPORT :
*  Enable exporting of functions when building a Windows DLL
*/
#if defined(LZ5_DLL_EXPORT) && (LZ5_DLL_EXPORT==1)
#  define LZ5LIB_API __declspec(dllexport)
#elif defined(LZ5_DLL_IMPORT) && (LZ5_DLL_IMPORT==1)
#  define LZ5LIB_API __declspec(dllimport) /* It isn't required but allows to generate better code, saving a function pointer load from the IAT and an indirect jump.*/
#else
#  define LZ5LIB_API
#endif


/*-************************************
*  Version
**************************************/
#define LZ5_VERSION_MAJOR    2    /* for breaking interface changes  */
#define LZ5_VERSION_MINOR    0    /* for new (non-breaking) interface capabilities */
#define LZ5_VERSION_RELEASE  0    /* for tweaks, bug-fixes, or development */

#define LZ5_VERSION_NUMBER (LZ5_VERSION_MAJOR *100*100 + LZ5_VERSION_MINOR *100 + LZ5_VERSION_RELEASE)
int LZ5_versionNumber (void);

#define LZ5_LIB_VERSION LZ5_VERSION_MAJOR.LZ5_VERSION_MINOR.LZ5_VERSION_RELEASE
#define LZ5_QUOTE(str) #str
#define LZ5_EXPAND_AND_QUOTE(str) LZ5_QUOTE(str)
#define LZ5_VERSION_STRING LZ5_EXPAND_AND_QUOTE(LZ5_LIB_VERSION)
const char* LZ5_versionString (void);

typedef struct LZ5_stream_s LZ5_stream_t;

#define LZ5_MIN_CLEVEL      10  /* minimum compression level */
#ifndef LZ5_NO_HUFFMAN
    #define LZ5_MAX_CLEVEL      49  /* maximum compression level */
#else
    #define LZ5_MAX_CLEVEL      29  /* maximum compression level */
#endif
#define LZ5_DEFAULT_CLEVEL  17


/*-************************************
*  Simple Functions
**************************************/

LZ5LIB_API int LZ5_compress (const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel); 

/*
LZ5_compress() :
    Compresses 'sourceSize' bytes from buffer 'source'
    into already allocated 'dest' buffer of size 'maxDestSize'.
    Compression is guaranteed to succeed if 'maxDestSize' >= LZ5_compressBound(sourceSize).
    It also runs faster, so it's a recommended setting.
    If the function cannot compress 'source' into a more limited 'dest' budget,
    compression stops *immediately*, and the function result is zero.
    As a consequence, 'dest' content is not valid.
    This function never writes outside 'dest' buffer, nor read outside 'source' buffer.
        sourceSize  : Max supported value is LZ5_MAX_INPUT_VALUE
        maxDestSize : full or partial size of buffer 'dest' (which must be already allocated)
        return : the number of bytes written into buffer 'dest' (necessarily <= maxOutputSize)
              or 0 if compression fails
*/


/*-************************************
*  Advanced Functions
**************************************/
#define LZ5_MAX_INPUT_SIZE  0x7E000000   /* 2 113 929 216 bytes */
#define LZ5_BLOCK_SIZE      (1<<17)
#define LZ5_BLOCK64K_SIZE   (1<<16)
#define LZ5_COMPRESSBOUND(isize)  ((unsigned)(isize) > (unsigned)LZ5_MAX_INPUT_SIZE ? 0 : (isize) + 1 + 1 + ((isize/LZ5_BLOCK_SIZE)+1)*4)


/*!
LZ5_compressBound() :
    Provides the maximum size that LZ5 compression may output in a "worst case" scenario (input data not compressible)
    This function is primarily useful for memory allocation purposes (destination buffer size).
    Macro LZ5_COMPRESSBOUND() is also provided for compilation-time evaluation (stack memory allocation for example).
    Note that LZ5_compress() compress faster when dest buffer size is >= LZ5_compressBound(srcSize)
        inputSize  : max supported value is LZ5_MAX_INPUT_SIZE
        return : maximum output size in a "worst case" scenario
              or 0, if input size is too large ( > LZ5_MAX_INPUT_SIZE)
*/
LZ5LIB_API int LZ5_compressBound(int inputSize);


/*!
LZ5_compress_extState() :
    Same compression function, just using an externally allocated memory space to store compression state.
    Use LZ5_sizeofState() to know how much memory must be allocated,
    and allocate it on 8-bytes boundaries (using malloc() typically).
    Then, provide it as 'void* state' to compression function.
*/
LZ5LIB_API int LZ5_sizeofState(int compressionLevel); 

LZ5LIB_API int LZ5_compress_extState(void* state, const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel);



/*-*********************************************
*  Streaming Compression Functions
***********************************************/

/*! LZ5_createStream() will allocate and initialize an `LZ5_stream_t` structure.
 *  LZ5_freeStream() releases its memory.
 *  In the context of a DLL (liblz5), please use these methods rather than the static struct.
 *  They are more future proof, in case of a change of `LZ5_stream_t` size.
 */
LZ5LIB_API LZ5_stream_t* LZ5_createStream(int compressionLevel);
LZ5LIB_API int           LZ5_freeStream (LZ5_stream_t* streamPtr);


/*! LZ5_resetStream() :
 *  Use this function to reset/reuse an allocated `LZ5_stream_t` structure
 */
LZ5LIB_API LZ5_stream_t* LZ5_resetStream (LZ5_stream_t* streamPtr, int compressionLevel); 


/*! LZ5_loadDict() :
 *  Use this function to load a static dictionary into LZ5_stream.
 *  Any previous data will be forgotten, only 'dictionary' will remain in memory.
 *  Loading a size of 0 is allowed.
 *  Return : dictionary size, in bytes (necessarily <= LZ5_DICT_SIZE)
 */
LZ5LIB_API int LZ5_loadDict (LZ5_stream_t* streamPtr, const char* dictionary, int dictSize);


/*! LZ5_compress_continue() :
 *  Compress buffer content 'src', using data from previously compressed blocks as dictionary to improve compression ratio.
 *  Important : Previous data blocks are assumed to still be present and unmodified !
 *  'dst' buffer must be already allocated.
 *  If maxDstSize >= LZ5_compressBound(srcSize), compression is guaranteed to succeed, and runs faster.
 *  If not, and if compressed data cannot fit into 'dst' buffer size, compression stops, and function returns a zero.
 */
LZ5LIB_API int LZ5_compress_continue (LZ5_stream_t* streamPtr, const char* src, char* dst, int srcSize, int maxDstSize);


/*! LZ5_saveDict() :
 *  If previously compressed data block is not guaranteed to remain available at its memory location,
 *  save it into a safer place (char* safeBuffer).
 *  Note : you don't need to call LZ5_loadDict() afterwards,
 *         dictionary is immediately usable, you can therefore call LZ5_compress_continue().
 *  Return : saved dictionary size in bytes (necessarily <= dictSize), or 0 if error.
 */
LZ5LIB_API int LZ5_saveDict (LZ5_stream_t* streamPtr, char* safeBuffer, int dictSize);





#if defined (__cplusplus)
}
#endif

#endif /* LZ5_H_2983827168210 */
