/* ******************************************************************
   FSE : Finite State Entropy codec
   Public Prototypes declaration
   Copyright (C) 2013-2016, Yann Collet.

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
   - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
****************************************************************** */
#ifndef LIZFSE_H
#define LIZFSE_H

#if defined (__cplusplus)
extern "C" {
#endif


/*-*****************************************
*  Dependencies
******************************************/
#include <stddef.h>    /* size_t, ptrdiff_t */


/*-****************************************
*  FSE simple functions
******************************************/
/*! LIZFSE_compress() :
    Compress content of buffer 'src', of size 'srcSize', into destination buffer 'dst'.
    'dst' buffer must be already allocated. Compression runs faster is dstCapacity >= LIZFSE_compressBound(srcSize).
    @return : size of compressed data (<= dstCapacity).
    Special values : if return == 0, srcData is not compressible => Nothing is stored within dst !!!
                     if return == 1, srcData is a single byte symbol * srcSize times. Use RLE compression instead.
                     if LIZFSE_isError(return), compression failed (more details using LIZFSE_getErrorName())
*/
size_t LIZFSE_compress(void* dst, size_t dstCapacity,
              const void* src, size_t srcSize);

/*! LIZFSE_decompress():
    Decompress FSE data from buffer 'cSrc', of size 'cSrcSize',
    into already allocated destination buffer 'dst', of size 'dstCapacity'.
    @return : size of regenerated data (<= maxDstSize),
              or an error code, which can be tested using LIZFSE_isError() .

    ** Important ** : LIZFSE_decompress() does not decompress non-compressible nor RLE data !!!
    Why ? : making this distinction requires a header.
    Header management is intentionally delegated to the user layer, which can better manage special cases.
*/
size_t LIZFSE_decompress(void* dst,  size_t dstCapacity,
                const void* cSrc, size_t cSrcSize);


/*-*****************************************
*  Tool functions
******************************************/
size_t LIZFSE_compressBound(size_t size);       /* maximum compressed size */

/* Error Management */
unsigned    LIZFSE_isError(size_t code);        /* tells if a return value is an error code */
const char* LIZFSE_getErrorName(size_t code);   /* provides error code string (useful for debugging) */


/*-*****************************************
*  FSE advanced functions
******************************************/
/*! LIZFSE_compress2() :
    Same as LIZFSE_compress(), but allows the selection of 'maxSymbolValue' and 'tableLog'
    Both parameters can be defined as '0' to mean : use default value
    @return : size of compressed data
    Special values : if return == 0, srcData is not compressible => Nothing is stored within cSrc !!!
                     if return == 1, srcData is a single byte symbol * srcSize times. Use RLE compression.
                     if LIZFSE_isError(return), it's an error code.
*/
size_t LIZFSE_compress2 (void* dst, size_t dstSize, const void* src, size_t srcSize, unsigned maxSymbolValue, unsigned tableLog);


/*-*****************************************
*  FSE detailed API
******************************************/
/*!
LIZFSE_compress() does the following:
1. count symbol occurrence from source[] into table count[]
2. normalize counters so that sum(count[]) == Power_of_2 (2^tableLog)
3. save normalized counters to memory buffer using writeNCount()
4. build encoding table 'CTable' from normalized counters
5. encode the data stream using encoding table 'CTable'

LIZFSE_decompress() does the following:
1. read normalized counters with readNCount()
2. build decoding table 'DTable' from normalized counters
3. decode the data stream using decoding table 'DTable'

The following API allows targeting specific sub-functions for advanced tasks.
For example, it's possible to compress several blocks using the same 'CTable',
or to save and provide normalized distribution using external method.
*/

/* *** COMPRESSION *** */

/*! LIZFSE_count():
    Provides the precise count of each byte within a table 'count'.
    'count' is a table of unsigned int, of minimum size (*maxSymbolValuePtr+1).
    *maxSymbolValuePtr will be updated if detected smaller than initial value.
    @return : the count of the most frequent symbol (which is not identified).
              if return == srcSize, there is only one symbol.
              Can also return an error code, which can be tested with LIZFSE_isError(). */
size_t LIZFSE_count(unsigned* count, unsigned* maxSymbolValuePtr, const void* src, size_t srcSize);

/*! LIZFSE_optimalTableLog():
    dynamically downsize 'tableLog' when conditions are met.
    It saves CPU time, by using smaller tables, while preserving or even improving compression ratio.
    @return : recommended tableLog (necessarily <= 'maxTableLog') */
unsigned LIZFSE_optimalTableLog(unsigned maxTableLog, size_t srcSize, unsigned maxSymbolValue);

/*! LIZFSE_normalizeCount():
    normalize counts so that sum(count[]) == Power_of_2 (2^tableLog)
    'normalizedCounter' is a table of short, of minimum size (maxSymbolValue+1).
    @return : tableLog,
              or an errorCode, which can be tested using LIZFSE_isError() */
size_t LIZFSE_normalizeCount(short* normalizedCounter, unsigned tableLog, const unsigned* count, size_t srcSize, unsigned maxSymbolValue);

/*! LIZFSE_NCountWriteBound():
    Provides the maximum possible size of an FSE normalized table, given 'maxSymbolValue' and 'tableLog'.
    Typically useful for allocation purpose. */
size_t LIZFSE_NCountWriteBound(unsigned maxSymbolValue, unsigned tableLog);

/*! LIZFSE_writeNCount():
    Compactly save 'normalizedCounter' into 'buffer'.
    @return : size of the compressed table,
              or an errorCode, which can be tested using LIZFSE_isError(). */
size_t LIZFSE_writeNCount (void* buffer, size_t bufferSize, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog);


/*! Constructor and Destructor of LIZFSE_CTable.
    Note that LIZFSE_CTable size depends on 'tableLog' and 'maxSymbolValue' */
typedef unsigned LIZFSE_CTable;   /* don't allocate that. It's only meant to be more restrictive than void* */
LIZFSE_CTable* LIZFSE_createCTable (unsigned tableLog, unsigned maxSymbolValue);
void        LIZFSE_freeCTable (LIZFSE_CTable* ct);

/*! LIZFSE_buildCTable():
    Builds `ct`, which must be already allocated, using LIZFSE_createCTable().
    @return : 0, or an errorCode, which can be tested using LIZFSE_isError() */
size_t LIZFSE_buildCTable(LIZFSE_CTable* ct, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog);

/*! LIZFSE_compress_usingCTable():
    Compress `src` using `ct` into `dst` which must be already allocated.
    @return : size of compressed data (<= `dstCapacity`),
              or 0 if compressed data could not fit into `dst`,
              or an errorCode, which can be tested using LIZFSE_isError() */
size_t LIZFSE_compress_usingCTable (void* dst, size_t dstCapacity, const void* src, size_t srcSize, const LIZFSE_CTable* ct);

/*!
Tutorial :
----------
The first step is to count all symbols. LIZFSE_count() does this job very fast.
Result will be saved into 'count', a table of unsigned int, which must be already allocated, and have 'maxSymbolValuePtr[0]+1' cells.
'src' is a table of bytes of size 'srcSize'. All values within 'src' MUST be <= maxSymbolValuePtr[0]
maxSymbolValuePtr[0] will be updated, with its real value (necessarily <= original value)
LIZFSE_count() will return the number of occurrence of the most frequent symbol.
This can be used to know if there is a single symbol within 'src', and to quickly evaluate its compressibility.
If there is an error, the function will return an ErrorCode (which can be tested using LIZFSE_isError()).

The next step is to normalize the frequencies.
LIZFSE_normalizeCount() will ensure that sum of frequencies is == 2 ^'tableLog'.
It also guarantees a minimum of 1 to any Symbol with frequency >= 1.
You can use 'tableLog'==0 to mean "use default tableLog value".
If you are unsure of which tableLog value to use, you can ask LIZFSE_optimalTableLog(),
which will provide the optimal valid tableLog given sourceSize, maxSymbolValue, and a user-defined maximum (0 means "default").

The result of LIZFSE_normalizeCount() will be saved into a table,
called 'normalizedCounter', which is a table of signed short.
'normalizedCounter' must be already allocated, and have at least 'maxSymbolValue+1' cells.
The return value is tableLog if everything proceeded as expected.
It is 0 if there is a single symbol within distribution.
If there is an error (ex: invalid tableLog value), the function will return an ErrorCode (which can be tested using LIZFSE_isError()).

'normalizedCounter' can be saved in a compact manner to a memory area using LIZFSE_writeNCount().
'buffer' must be already allocated.
For guaranteed success, buffer size must be at least LIZFSE_headerBound().
The result of the function is the number of bytes written into 'buffer'.
If there is an error, the function will return an ErrorCode (which can be tested using LIZFSE_isError(); ex : buffer size too small).

'normalizedCounter' can then be used to create the compression table 'CTable'.
The space required by 'CTable' must be already allocated, using LIZFSE_createCTable().
You can then use LIZFSE_buildCTable() to fill 'CTable'.
If there is an error, both functions will return an ErrorCode (which can be tested using LIZFSE_isError()).

'CTable' can then be used to compress 'src', with LIZFSE_compress_usingCTable().
Similar to LIZFSE_count(), the convention is that 'src' is assumed to be a table of char of size 'srcSize'
The function returns the size of compressed data (without header), necessarily <= `dstCapacity`.
If it returns '0', compressed data could not fit into 'dst'.
If there is an error, the function will return an ErrorCode (which can be tested using LIZFSE_isError()).
*/


/* *** DECOMPRESSION *** */

/*! LIZFSE_readNCount():
    Read compactly saved 'normalizedCounter' from 'rBuffer'.
    @return : size read from 'rBuffer',
              or an errorCode, which can be tested using LIZFSE_isError().
              maxSymbolValuePtr[0] and tableLogPtr[0] will also be updated with their respective values */
size_t LIZFSE_readNCount (short* normalizedCounter, unsigned* maxSymbolValuePtr, unsigned* tableLogPtr, const void* rBuffer, size_t rBuffSize);

/*! Constructor and Destructor of LIZFSE_DTable.
    Note that its size depends on 'tableLog' */
typedef unsigned LIZFSE_DTable;   /* don't allocate that. It's just a way to be more restrictive than void* */
LIZFSE_DTable* LIZFSE_createDTable(unsigned tableLog);
void        LIZFSE_freeDTable(LIZFSE_DTable* dt);

/*! LIZFSE_buildDTable():
    Builds 'dt', which must be already allocated, using LIZFSE_createDTable().
    return : 0, or an errorCode, which can be tested using LIZFSE_isError() */
size_t LIZFSE_buildDTable (LIZFSE_DTable* dt, const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog);

/*! LIZFSE_decompress_usingDTable():
    Decompress compressed source `cSrc` of size `cSrcSize` using `dt`
    into `dst` which must be already allocated.
    @return : size of regenerated data (necessarily <= `dstCapacity`),
              or an errorCode, which can be tested using LIZFSE_isError() */
size_t LIZFSE_decompress_usingDTable(void* dst, size_t dstCapacity, const void* cSrc, size_t cSrcSize, const LIZFSE_DTable* dt);

/*!
Tutorial :
----------
(Note : these functions only decompress FSE-compressed blocks.
 If block is uncompressed, use memcpy() instead
 If block is a single repeated byte, use memset() instead )

The first step is to obtain the normalized frequencies of symbols.
This can be performed by LIZFSE_readNCount() if it was saved using LIZFSE_writeNCount().
'normalizedCounter' must be already allocated, and have at least 'maxSymbolValuePtr[0]+1' cells of signed short.
In practice, that means it's necessary to know 'maxSymbolValue' beforehand,
or size the table to handle worst case situations (typically 256).
LIZFSE_readNCount() will provide 'tableLog' and 'maxSymbolValue'.
The result of LIZFSE_readNCount() is the number of bytes read from 'rBuffer'.
Note that 'rBufferSize' must be at least 4 bytes, even if useful information is less than that.
If there is an error, the function will return an error code, which can be tested using LIZFSE_isError().

The next step is to build the decompression tables 'LIZFSE_DTable' from 'normalizedCounter'.
This is performed by the function LIZFSE_buildDTable().
The space required by 'LIZFSE_DTable' must be already allocated using LIZFSE_createDTable().
If there is an error, the function will return an error code, which can be tested using LIZFSE_isError().

`LIZFSE_DTable` can then be used to decompress `cSrc`, with LIZFSE_decompress_usingDTable().
`cSrcSize` must be strictly correct, otherwise decompression will fail.
LIZFSE_decompress_usingDTable() result will tell how many bytes were regenerated (<=`dstCapacity`).
If there is an error, the function will return an error code, which can be tested using LIZFSE_isError(). (ex: dst buffer too small)
*/


#ifdef LIZFSE_STATIC_LINKING_ONLY

/* *** Dependency *** */
#include "bitstream.h"


/* *****************************************
*  Static allocation
*******************************************/
/* FSE buffer bounds */
#define LIZFSE_NCOUNTBOUND 512
#define LIZFSE_BLOCKBOUND(size) (size + (size>>7))
#define LIZFSE_COMPRESSBOUND(size) (LIZFSE_NCOUNTBOUND + LIZFSE_BLOCKBOUND(size))   /* Macro version, useful for static allocation */

/* It is possible to statically allocate FSE CTable/DTable as a table of unsigned using below macros */
#define LIZFSE_CTABLE_SIZE_U32(maxTableLog, maxSymbolValue)   (1 + (1<<(maxTableLog-1)) + ((maxSymbolValue+1)*2))
#define LIZFSE_DTABLE_SIZE_U32(maxTableLog)                   (1 + (1<<maxTableLog))


/* *****************************************
*  FSE advanced API
*******************************************/
size_t LIZFSE_countFast(unsigned* count, unsigned* maxSymbolValuePtr, const void* src, size_t srcSize);
/**< same as LIZFSE_count(), but blindly trusts that all byte values within src are <= *maxSymbolValuePtr  */

unsigned LIZFSE_optimalTableLog_internal(unsigned maxTableLog, size_t srcSize, unsigned maxSymbolValue, unsigned minus);
/**< same as LIZFSE_optimalTableLog(), which used `minus==2` */

size_t LIZFSE_buildCTable_raw (LIZFSE_CTable* ct, unsigned nbBits);
/**< build a fake LIZFSE_CTable, designed to not compress an input, where each symbol uses nbBits */

size_t LIZFSE_buildCTable_rle (LIZFSE_CTable* ct, unsigned char symbolValue);
/**< build a fake LIZFSE_CTable, designed to compress always the same symbolValue */

size_t LIZFSE_buildDTable_raw (LIZFSE_DTable* dt, unsigned nbBits);
/**< build a fake LIZFSE_DTable, designed to read an uncompressed bitstream where each symbol uses nbBits */

size_t LIZFSE_buildDTable_rle (LIZFSE_DTable* dt, unsigned char symbolValue);
/**< build a fake LIZFSE_DTable, designed to always generate the same symbolValue */


/* *****************************************
*  FSE symbol compression API
*******************************************/
/*!
   This API consists of small unitary functions, which highly benefit from being inlined.
   You will want to enable link-time-optimization to ensure these functions are properly inlined in your binary.
   Visual seems to do it automatically.
   For gcc or clang, you'll need to add -flto flag at compilation and linking stages.
   If none of these solutions is applicable, include "fse.c" directly.
*/
typedef struct
{
    ptrdiff_t   value;
    const void* stateTable;
    const void* symbolTT;
    unsigned    stateLog;
} LIZFSE_CState_t;

static void LIZFSE_initCState(LIZFSE_CState_t* CStatePtr, const LIZFSE_CTable* ct);

static void LIZFSE_encodeSymbol(BIT_CStream_t* bitC, LIZFSE_CState_t* CStatePtr, unsigned symbol);

static void LIZFSE_flushCState(BIT_CStream_t* bitC, const LIZFSE_CState_t* CStatePtr);

/**<
These functions are inner components of LIZFSE_compress_usingCTable().
They allow the creation of custom streams, mixing multiple tables and bit sources.

A key property to keep in mind is that encoding and decoding are done **in reverse direction**.
So the first symbol you will encode is the last you will decode, like a LIFO stack.

You will need a few variables to track your CStream. They are :

LIZFSE_CTable    ct;         // Provided by LIZFSE_buildCTable()
BIT_CStream_t bitStream;  // bitStream tracking structure
LIZFSE_CState_t  state;      // State tracking structure (can have several)


The first thing to do is to init bitStream and state.
    size_t errorCode = BIT_initCStream(&bitStream, dstBuffer, maxDstSize);
    LIZFSE_initCState(&state, ct);

Note that BIT_initCStream() can produce an error code, so its result should be tested, using LIZFSE_isError();
You can then encode your input data, byte after byte.
LIZFSE_encodeSymbol() outputs a maximum of 'tableLog' bits at a time.
Remember decoding will be done in reverse direction.
    LIZFSE_encodeByte(&bitStream, &state, symbol);

At any time, you can also add any bit sequence.
Note : maximum allowed nbBits is 25, for compatibility with 32-bits decoders
    BIT_addBits(&bitStream, bitField, nbBits);

The above methods don't commit data to memory, they just store it into local register, for speed.
Local register size is 64-bits on 64-bits systems, 32-bits on 32-bits systems (size_t).
Writing data to memory is a manual operation, performed by the flushBits function.
    BIT_flushBits(&bitStream);

Your last FSE encoding operation shall be to flush your last state value(s).
    LIZFSE_flushState(&bitStream, &state);

Finally, you must close the bitStream.
The function returns the size of CStream in bytes.
If data couldn't fit into dstBuffer, it will return a 0 ( == not compressible)
If there is an error, it returns an errorCode (which can be tested using LIZFSE_isError()).
    size_t size = BIT_closeCStream(&bitStream);
*/


/* *****************************************
*  FSE symbol decompression API
*******************************************/
typedef struct
{
    size_t      state;
    const void* table;   /* precise table may vary, depending on U16 */
} LIZFSE_DState_t;


static void     LIZFSE_initDState(LIZFSE_DState_t* DStatePtr, BIT_DStream_t* bitD, const LIZFSE_DTable* dt);

static unsigned char LIZFSE_decodeSymbol(LIZFSE_DState_t* DStatePtr, BIT_DStream_t* bitD);

static unsigned LIZFSE_endOfDState(const LIZFSE_DState_t* DStatePtr);

/**<
Let's now decompose LIZFSE_decompress_usingDTable() into its unitary components.
You will decode FSE-encoded symbols from the bitStream,
and also any other bitFields you put in, **in reverse order**.

You will need a few variables to track your bitStream. They are :

BIT_DStream_t DStream;    // Stream context
LIZFSE_DState_t  DState;     // State context. Multiple ones are possible
LIZFSE_DTable*   DTablePtr;  // Decoding table, provided by LIZFSE_buildDTable()

The first thing to do is to init the bitStream.
    errorCode = BIT_initDStream(&DStream, srcBuffer, srcSize);

You should then retrieve your initial state(s)
(in reverse flushing order if you have several ones) :
    errorCode = LIZFSE_initDState(&DState, &DStream, DTablePtr);

You can then decode your data, symbol after symbol.
For information the maximum number of bits read by LIZFSE_decodeSymbol() is 'tableLog'.
Keep in mind that symbols are decoded in reverse order, like a LIFO stack (last in, first out).
    unsigned char symbol = LIZFSE_decodeSymbol(&DState, &DStream);

You can retrieve any bitfield you eventually stored into the bitStream (in reverse order)
Note : maximum allowed nbBits is 25, for 32-bits compatibility
    size_t bitField = BIT_readBits(&DStream, nbBits);

All above operations only read from local register (which size depends on size_t).
Refueling the register from memory is manually performed by the reload method.
    endSignal = LIZFSE_reloadDStream(&DStream);

BIT_reloadDStream() result tells if there is still some more data to read from DStream.
BIT_DStream_unfinished : there is still some data left into the DStream.
BIT_DStream_endOfBuffer : Dstream reached end of buffer. Its container may no longer be completely filled.
BIT_DStream_completed : Dstream reached its exact end, corresponding in general to decompression completed.
BIT_DStream_tooFar : Dstream went too far. Decompression result is corrupted.

When reaching end of buffer (BIT_DStream_endOfBuffer), progress slowly, notably if you decode multiple symbols per loop,
to properly detect the exact end of stream.
After each decoded symbol, check if DStream is fully consumed using this simple test :
    BIT_reloadDStream(&DStream) >= BIT_DStream_completed

When it's done, verify decompression is fully completed, by checking both DStream and the relevant states.
Checking if DStream has reached its end is performed by :
    BIT_endOfDStream(&DStream);
Check also the states. There might be some symbols left there, if some high probability ones (>50%) are possible.
    LIZFSE_endOfDState(&DState);
*/


/* *****************************************
*  FSE unsafe API
*******************************************/
static unsigned char LIZFSE_decodeSymbolFast(LIZFSE_DState_t* DStatePtr, BIT_DStream_t* bitD);
/* faster, but works only if nbBits is always >= 1 (otherwise, result will be corrupted) */


/* *****************************************
*  Implementation of inlined functions
*******************************************/
typedef struct {
    int deltaFindState;
    U32 deltaNbBits;
} LIZFSE_symbolCompressionTransform; /* total 8 bytes */

MEM_STATIC void LIZFSE_initCState(LIZFSE_CState_t* statePtr, const LIZFSE_CTable* ct)
{
    const void* ptr = ct;
    const U16* u16ptr = (const U16*) ptr;
    const U32 tableLog = MEM_read16(ptr);
    statePtr->value = (ptrdiff_t)1<<tableLog;
    statePtr->stateTable = u16ptr+2;
    statePtr->symbolTT = ((const U32*)ct + 1 + (tableLog ? (1<<(tableLog-1)) : 1));
    statePtr->stateLog = tableLog;
}


/*! LIZFSE_initCState2() :
*   Same as LIZFSE_initCState(), but the first symbol to include (which will be the last to be read)
*   uses the smallest state value possible, saving the cost of this symbol */
MEM_STATIC void LIZFSE_initCState2(LIZFSE_CState_t* statePtr, const LIZFSE_CTable* ct, U32 symbol)
{
    LIZFSE_initCState(statePtr, ct);
    {   const LIZFSE_symbolCompressionTransform symbolTT = ((const LIZFSE_symbolCompressionTransform*)(statePtr->symbolTT))[symbol];
        const U16* stateTable = (const U16*)(statePtr->stateTable);
        U32 nbBitsOut  = (U32)((symbolTT.deltaNbBits + (1<<15)) >> 16);
        statePtr->value = (nbBitsOut << 16) - symbolTT.deltaNbBits;
        statePtr->value = stateTable[(statePtr->value >> nbBitsOut) + symbolTT.deltaFindState];
    }
}

MEM_STATIC void LIZFSE_encodeSymbol(BIT_CStream_t* bitC, LIZFSE_CState_t* statePtr, U32 symbol)
{
    const LIZFSE_symbolCompressionTransform symbolTT = ((const LIZFSE_symbolCompressionTransform*)(statePtr->symbolTT))[symbol];
    const U16* const stateTable = (const U16*)(statePtr->stateTable);
    U32 nbBitsOut  = (U32)((statePtr->value + symbolTT.deltaNbBits) >> 16);
    BIT_addBits(bitC, statePtr->value, nbBitsOut);
    statePtr->value = stateTable[ (statePtr->value >> nbBitsOut) + symbolTT.deltaFindState];
}

MEM_STATIC void LIZFSE_flushCState(BIT_CStream_t* bitC, const LIZFSE_CState_t* statePtr)
{
    BIT_addBits(bitC, statePtr->value, statePtr->stateLog);
    BIT_flushBits(bitC);
}

/* ======    Decompression    ====== */

typedef struct {
    U16 tableLog;
    U16 fastMode;
} LIZFSE_DTableHeader;   /* sizeof U32 */

typedef struct
{
    unsigned short newState;
    unsigned char  symbol;
    unsigned char  nbBits;
} LIZFSE_decode_t;   /* size == U32 */

MEM_STATIC void LIZFSE_initDState(LIZFSE_DState_t* DStatePtr, BIT_DStream_t* bitD, const LIZFSE_DTable* dt)
{
    const void* ptr = dt;
    const LIZFSE_DTableHeader* const DTableH = (const LIZFSE_DTableHeader*)ptr;
    DStatePtr->state = BIT_readBits(bitD, DTableH->tableLog);
    BIT_reloadDStream(bitD);
    DStatePtr->table = dt + 1;
}

MEM_STATIC BYTE LIZFSE_peekSymbol(const LIZFSE_DState_t* DStatePtr)
{
    LIZFSE_decode_t const DInfo = ((const LIZFSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    return DInfo.symbol;
}

MEM_STATIC void LIZFSE_updateState(LIZFSE_DState_t* DStatePtr, BIT_DStream_t* bitD)
{
    LIZFSE_decode_t const DInfo = ((const LIZFSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    U32 const nbBits = DInfo.nbBits;
    size_t const lowBits = BIT_readBits(bitD, nbBits);
    DStatePtr->state = DInfo.newState + lowBits;
}

MEM_STATIC BYTE LIZFSE_decodeSymbol(LIZFSE_DState_t* DStatePtr, BIT_DStream_t* bitD)
{
    LIZFSE_decode_t const DInfo = ((const LIZFSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    U32 const nbBits = DInfo.nbBits;
    BYTE const symbol = DInfo.symbol;
    size_t const lowBits = BIT_readBits(bitD, nbBits);

    DStatePtr->state = DInfo.newState + lowBits;
    return symbol;
}

/*! LIZFSE_decodeSymbolFast() :
    unsafe, only works if no symbol has a probability > 50% */
MEM_STATIC BYTE LIZFSE_decodeSymbolFast(LIZFSE_DState_t* DStatePtr, BIT_DStream_t* bitD)
{
    LIZFSE_decode_t const DInfo = ((const LIZFSE_decode_t*)(DStatePtr->table))[DStatePtr->state];
    U32 const nbBits = DInfo.nbBits;
    BYTE const symbol = DInfo.symbol;
    size_t const lowBits = BIT_readBitsFast(bitD, nbBits);

    DStatePtr->state = DInfo.newState + lowBits;
    return symbol;
}

MEM_STATIC unsigned LIZFSE_endOfDState(const LIZFSE_DState_t* DStatePtr)
{
    return DStatePtr->state == 0;
}



#ifndef LIZFSE_COMMONDEFS_ONLY

/* **************************************************************
*  Tuning parameters
****************************************************************/
/*!MEMORY_USAGE :
*  Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
*  Increasing memory usage improves compression ratio
*  Reduced memory usage can improve speed, due to cache effect
*  Recommended max value is 14, for 16KB, which nicely fits into Intel x86 L1 cache */
#define LIZFSE_MAX_MEMORY_USAGE 14
#define LIZFSE_DEFAULT_MEMORY_USAGE 13

/*!LIZFSE_MAX_SYMBOL_VALUE :
*  Maximum symbol value authorized.
*  Required for proper stack allocation */
#define LIZFSE_MAX_SYMBOL_VALUE 255


/* **************************************************************
*  template functions type & suffix
****************************************************************/
#define LIZFSE_FUNCTION_TYPE BYTE
#define LIZFSE_FUNCTION_EXTENSION
#define LIZFSE_DECODE_TYPE LIZFSE_decode_t


#endif   /* !LIZFSE_COMMONDEFS_ONLY */


/* ***************************************************************
*  Constants
*****************************************************************/
#define LIZFSE_MAX_TABLELOG  (LIZFSE_MAX_MEMORY_USAGE-2)
#define LIZFSE_MAX_TABLESIZE (1U<<LIZFSE_MAX_TABLELOG)
#define LIZFSE_MAXTABLESIZE_MASK (LIZFSE_MAX_TABLESIZE-1)
#define LIZFSE_DEFAULT_TABLELOG (LIZFSE_DEFAULT_MEMORY_USAGE-2)
#define LIZFSE_MIN_TABLELOG 5

#define LIZFSE_TABLELOG_ABSOLUTE_MAX 15
#if LIZFSE_MAX_TABLELOG > LIZFSE_TABLELOG_ABSOLUTE_MAX
#  error "LIZFSE_MAX_TABLELOG > LIZFSE_TABLELOG_ABSOLUTE_MAX is not supported"
#endif

#define LIZFSE_TABLESTEP(tableSize) ((tableSize>>1) + (tableSize>>3) + 3)


#endif /* LIZFSE_STATIC_LINKING_ONLY */


#if defined (__cplusplus)
}
#endif

#endif  /* LIZFSE_H */
