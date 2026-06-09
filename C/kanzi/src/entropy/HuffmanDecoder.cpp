/*
Copyright 2011-2026 Frederic Langlet
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
you may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <algorithm>
#include <sstream>
#include "HuffmanDecoder.hpp"
#include "EntropyUtils.hpp"
#include "ExpGolombDecoder.hpp"
#include "../BitStreamException.hpp"
#include "../Memory.hpp"

using namespace kanzi;
using namespace std;

const int HuffmanDecoder::DECODING_BATCH_SIZE = 12; // ensures decoding table fits in L1 cache
const int HuffmanDecoder::TABLE_MASK = (1 << DECODING_BATCH_SIZE) - 1;


// The chunk size indicates how many bytes are encoded (per block) before
// resetting the frequency stats.
HuffmanDecoder::HuffmanDecoder(InputBitStream& bitstream, Context* pCtx, int chunkSize) : _bitstream(bitstream)
{
    if (chunkSize < 1024)
        throw invalid_argument("Huffman codec: The chunk size must be at least 1024");

    if (chunkSize > HuffmanCommon::MAX_CHUNK_SIZE) {
        stringstream ss;
        ss << "Huffman codec: The chunk size must be at most " << HuffmanCommon::MAX_CHUNK_SIZE;
        throw invalid_argument(ss.str());
    }

    _chunkSize = chunkSize;
    _buffer = nullptr;
    _bufferSize = 0;
    _pCtx = pCtx;
    reset();
}

bool HuffmanDecoder::reset()
{
    // Default lengths & canonical codes
    for (uint16 i = 0; i < 256; i++) {
        _codes[i] = i;
        _sizes[i] = 8;
    }

    memset(_alphabet, 0, sizeof(_alphabet));
    memset(_table, 0, sizeof(_table));
    return true;
}

int HuffmanDecoder::readLengths()
{
    const int count = EntropyUtils::decodeAlphabet(_bitstream, _alphabet);

    if (count == 0)
        return 0;

    ExpGolombDecoder egdec(_bitstream, true);
    int8 curSize = 2;

    // Read lengths from bitstream
    for (int i = 0; i < count; i++) {
        const uint s = _alphabet[i];

        if (s > 255) {
            stringstream ss;
            ss << "Invalid bitstream: incorrect Huffman symbol " << s;
            throw BitStreamException(ss.str(), BitStreamException::INVALID_STREAM);
        }

        _codes[s] = 0;
        curSize += int8(egdec.decodeByte());

        if ((curSize <= 0) || (curSize > HuffmanCommon::MAX_SYMBOL_SIZE)) {
            stringstream ss;
            ss << "Invalid bitstream: incorrect size " << int(curSize);
            ss << " for Huffman symbol " << s;
            throw BitStreamException(ss.str(), BitStreamException::INVALID_STREAM);
        }

        _sizes[s] = uint16(curSize);
    }

    // Create canonical codes
    if (HuffmanCommon::generateCanonicalCodes(_sizes, _codes, _alphabet, count) < 0) {
        stringstream ss;
        ss << "Could not generate Huffman codes: max code length (";
        ss << HuffmanCommon::MAX_SYMBOL_SIZE;
        ss << " bits) exceeded";
        throw BitStreamException(ss.str(), BitStreamException::INVALID_STREAM);
    }

    return count;
}

// max(CodeLen) must be <= MAX_SYMBOL_SIZE
bool HuffmanDecoder::buildDecodingTable(int count)
{
    // Initialize table with non zero values.
    // If the bitstream is altered, the decoder may access these default table values.
    // The number of consumed bits cannot be 0.
    memset(_table, 7, sizeof(_table));
    uint16 length = 0;

    for (int i = 0; i < count; i++) {
        const uint s = _alphabet[i];

        // All DECODING_BATCH_SIZE bit values read from the bit stream and
        // starting with the same prefix point to symbol s
        length = max(_sizes[s], length);
        const int w = 1 << (DECODING_BATCH_SIZE - length);
        int idx = int(_codes[s]) * w;
        const int end = idx + w;

        if (end > TABLE_MASK + 1)
            return false;

        // code -> size, symbol
        const uint16 val = (uint16(s) << 8) | _sizes[s];

        while (idx < end)
            _table[idx++] = val;
    }

    return true;
}

int HuffmanDecoder::decode(kanzi::byte block[], uint blkptr, uint count)
{
    if (count == 0)
        return 0;

    int bsVersion = _pCtx == nullptr ? 6 : _pCtx->getInt("bsVersion", 6);

    if (bsVersion < 6)
        return decodeV5(block, blkptr, count);

    return decodeV6(block, blkptr, count);
}


int HuffmanDecoder::decodeV6(kanzi::byte block[], uint blkptr, uint count)
{
    const uint minBufSize = 2 * uint(_chunkSize);

    if (_bufferSize < minBufSize) {
        if (_buffer != nullptr)
           delete[] _buffer;

        _bufferSize = minBufSize;
        _buffer = new kanzi::byte[_bufferSize];
    }

    uint startChunk = blkptr;
    const uint end = blkptr + count;

    while (startChunk < end) {
        const uint sizeChunk = min(uint(_chunkSize), end - startChunk);

        if (sizeChunk < 32) {
            // Special case for small chunks
            _bitstream.readBits(&block[startChunk], 8 * sizeChunk);
        }
        else {
            // For each chunk, read code lengths, rebuild codes, rebuild decoding table
            const int alphabetSize = readLengths();

            if (alphabetSize <= 0)
                return startChunk - blkptr;

            if (alphabetSize == 1) {
                // Shortcut for chunks with only one symbol
                memset(&block[startChunk], _alphabet[0], size_t(sizeChunk));
            }
            else {
                if (buildDecodingTable(alphabetSize) == false)
                    return -1;

                if (decodeChunk(&block[startChunk], sizeChunk) == false)
                    return -1;
            }
        }

        startChunk += sizeChunk;
    }

    return count;
}

// count is at least 32
bool HuffmanDecoder::decodeChunk(kanzi::byte block[], uint count)
{
    // Read fragment sizes
    const int szBits0 = EntropyUtils::readVarInt(_bitstream);
    const int szBits1 = EntropyUtils::readVarInt(_bitstream);
    const int szBits2 = EntropyUtils::readVarInt(_bitstream);
    const int szBits3 = EntropyUtils::readVarInt(_bitstream);

    if ((szBits0 < 0) || (szBits1 < 0) || (szBits2 < 0) || (szBits3 < 0))
        return false;

    // Each of the 4 streams is stored in one quarter of _buffer.
    const int maxFragBits = int((_bufferSize >> 2) << 3);

    if ((szBits0 > maxFragBits) || (szBits1 > maxFragBits) || (szBits2 > maxFragBits) || (szBits3 > maxFragBits))
        return false;

    memset(_buffer, 0, _bufferSize);

    int idx0 = 0 * (_bufferSize / 4);
    int idx1 = 1 * (_bufferSize / 4);
    int idx2 = 2 * (_bufferSize / 4);
    int idx3 = 3 * (_bufferSize / 4);

    // Read all compressed data from bitstream
    _bitstream.readBits(&_buffer[idx0], szBits0);
    _bitstream.readBits(&_buffer[idx1], szBits1);
    _bitstream.readBits(&_buffer[idx2], szBits2);
    _bitstream.readBits(&_buffer[idx3], szBits3);

    // State variables for each of the four parallel streams
    uint64 state0 = 0, state1 = 0, state2 = 0, state3 = 0; // bits read from bitstream
    uint8 bits0 = 0, bits1 = 0, bits2 = 0, bits3 = 0;      // number of available bits in state

#define READ_STATE(shift, state, idx, bits) do {\
       const uint8 shift = (56 - bits) & -8; \
       bits += shift - DECODING_BATCH_SIZE; \
       state = (state << shift) | (uint64(BigEndian::readLong64(&_buffer[idx])) >> 1 >> (63 - shift)); /* handle shift = 0 */ \
       idx += (shift >> 3); \
    } while (0);

    const int szFrag = count / 4;
    kanzi::byte* block0 = &block[0 * szFrag];
    kanzi::byte* block1 = &block[1 * szFrag];
    kanzi::byte* block2 = &block[2 * szFrag];
    kanzi::byte* block3 = &block[3 * szFrag];
    int n = 0;

    while (n < szFrag - 4) {
        // Fill 64 bits of state from the bitstream for each stream
        READ_STATE(shift, state0, idx0, bits0);
        READ_STATE(shift, state1, idx1, bits1);
        READ_STATE(shift, state2, idx2, bits2);
        READ_STATE(shift, state3, idx3, bits3);

        // Decompress 4 symbols per stream
        const uint16 val00 = _table[(state0 >> bits0) & TABLE_MASK]; bits0 -= uint8(val00);
        const uint16 val10 = _table[(state1 >> bits1) & TABLE_MASK]; bits1 -= uint8(val10);
        const uint16 val20 = _table[(state2 >> bits2) & TABLE_MASK]; bits2 -= uint8(val20);
        const uint16 val30 = _table[(state3 >> bits3) & TABLE_MASK]; bits3 -= uint8(val30);
        const uint16 val01 = _table[(state0 >> bits0) & TABLE_MASK]; bits0 -= uint8(val01);
        const uint16 val11 = _table[(state1 >> bits1) & TABLE_MASK]; bits1 -= uint8(val11);
        const uint16 val21 = _table[(state2 >> bits2) & TABLE_MASK]; bits2 -= uint8(val21);
        const uint16 val31 = _table[(state3 >> bits3) & TABLE_MASK]; bits3 -= uint8(val31);
        const uint16 val02 = _table[(state0 >> bits0) & TABLE_MASK]; bits0 -= uint8(val02);
        const uint16 val12 = _table[(state1 >> bits1) & TABLE_MASK]; bits1 -= uint8(val12);
        const uint16 val22 = _table[(state2 >> bits2) & TABLE_MASK]; bits2 -= uint8(val22);
        const uint16 val32 = _table[(state3 >> bits3) & TABLE_MASK]; bits3 -= uint8(val32);
        const uint16 val03 = _table[(state0 >> bits0) & TABLE_MASK]; bits0 -= uint8(val03);
        const uint16 val13 = _table[(state1 >> bits1) & TABLE_MASK]; bits1 -= uint8(val13);
        const uint16 val23 = _table[(state2 >> bits2) & TABLE_MASK]; bits2 -= uint8(val23);
        const uint16 val33 = _table[(state3 >> bits3) & TABLE_MASK]; bits3 -= uint8(val33);

        bits0 += DECODING_BATCH_SIZE;
        bits1 += DECODING_BATCH_SIZE;
        bits2 += DECODING_BATCH_SIZE;
        bits3 += DECODING_BATCH_SIZE;

        block0[n + 0] = kanzi::byte(val00 >> 8);
        block1[n + 0] = kanzi::byte(val10 >> 8);
        block2[n + 0] = kanzi::byte(val20 >> 8);
        block3[n + 0] = kanzi::byte(val30 >> 8);
        block0[n + 1] = kanzi::byte(val01 >> 8);
        block1[n + 1] = kanzi::byte(val11 >> 8);
        block2[n + 1] = kanzi::byte(val21 >> 8);
        block3[n + 1] = kanzi::byte(val31 >> 8);
        block0[n + 2] = kanzi::byte(val02 >> 8);
        block1[n + 2] = kanzi::byte(val12 >> 8);
        block2[n + 2] = kanzi::byte(val22 >> 8);
        block3[n + 2] = kanzi::byte(val32 >> 8);
        block0[n + 3] = kanzi::byte(val03 >> 8);
        block1[n + 3] = kanzi::byte(val13 >> 8);
        block2[n + 3] = kanzi::byte(val23 >> 8);
        block3[n + 3] = kanzi::byte(val33 >> 8);
        n += 4;
    }

    // Fill 64 bits of state from the bitstream for each stream
    READ_STATE(shift, state0, idx0, bits0);
    READ_STATE(shift, state1, idx1, bits1);
    READ_STATE(shift, state2, idx2, bits2);
    READ_STATE(shift, state3, idx3, bits3);

    while (n < szFrag) {
        // Decompress 1 symbol per stream
        const uint16 val0 = _table[(state0 >> bits0) & TABLE_MASK]; bits0 -= uint8(val0);
        const uint16 val1 = _table[(state1 >> bits1) & TABLE_MASK]; bits1 -= uint8(val1);
        const uint16 val2 = _table[(state2 >> bits2) & TABLE_MASK]; bits2 -= uint8(val2);
        const uint16 val3 = _table[(state3 >> bits3) & TABLE_MASK]; bits3 -= uint8(val3);

        block0[n] = kanzi::byte(val0 >> 8);
        block1[n] = kanzi::byte(val1 >> 8);
        block2[n] = kanzi::byte(val2 >> 8);
        block3[n] = kanzi::byte(val3 >> 8);
        n++;
    }

    // Process any remaining bytes at the end of the whole chunk
    const uint count4 = 4 * szFrag;

    for (uint i = count4; i < count; i++)
        block[i] = kanzi::byte(_bitstream.readBits(8));

    return true;
}

int HuffmanDecoder::decodeV5(kanzi::byte block[], uint blkptr, uint count)
{
    uint startChunk = blkptr;
    const uint end = blkptr + count;

    while (startChunk < end) {
        const uint endChunk = min(startChunk + _chunkSize, end);
        const uint sizeChunk = endChunk - startChunk;

        // For each chunk, read code lengths, rebuild codes, rebuild decoding table
        const int alphabetSize = readLengths();

        if (alphabetSize <= 0)
            return startChunk - blkptr;

        if (alphabetSize == 1) {
            // Shortcut for chunks with only one symbol
            memset(&block[startChunk], _alphabet[0], size_t(endChunk - startChunk));
            startChunk = endChunk;
            continue;
        }

        if (buildDecodingTable(alphabetSize) == false)
            return -1;

        // Read number of streams. Only 1 steam supported for now
        if (_bitstream.readBits(2) != 0)
            return -1;

        // Read chunk size
        const int szBits = EntropyUtils::readVarInt(_bitstream);

        if ((szBits < 0) || (szBits > int(sizeChunk) * HuffmanCommon::MAX_SYMBOL_SIZE))
            return -1;

        // Read compressed data from bitstream
        if (szBits != 0) {
            const int sz = (szBits + 7) >> 3;
            const uint minLenBuf = uint(max(sz + (sz >> 3), 1024));

            if (_bufferSize < minLenBuf) {
                if (_buffer != nullptr)
                   delete[] _buffer;

                _bufferSize = minLenBuf;
                _buffer = new kanzi::byte[_bufferSize];
            }

            _bitstream.readBits(&_buffer[0], szBits);

            uint64 state = 0; // holds bits read from bitstream
            uint8 bits = 0; // number of available bits in state
            int idx = 0;
            uint n = startChunk;

            while (idx < sz - 8) {
                const uint8 shift = (56 - bits) & -8;
                state = (state << shift) | (uint64(BigEndian::readLong64(&_buffer[idx])) >> 1 >> (63 - shift)); // handle shift = 0
                idx += (shift >> 3);
                uint8 bs = bits + shift - DECODING_BATCH_SIZE;
                const uint16 val0 = _table[(state >> bs) & TABLE_MASK];
                bs -= uint8(val0);
                const uint16 val1 = _table[(state >> bs) & TABLE_MASK];
                bs -= uint8(val1);
                const uint16 val2 = _table[(state >> bs) & TABLE_MASK];
                bs -= uint8(val2);
                const uint16 val3 = _table[(state >> bs) & TABLE_MASK];
                bs -= uint8(val3);
                bits = bs + DECODING_BATCH_SIZE;
                block[n + 0] = kanzi::byte(val0 >> 8);
                block[n + 1] = kanzi::byte(val1 >> 8);
                block[n + 2] = kanzi::byte(val2 >> 8);
                block[n + 3] = kanzi::byte(val3 >> 8);
                n += 4;
            }

            // Last bytes
            uint nbBits = idx * 8;

            while (n < endChunk) {
                while ((bits < HuffmanCommon::MAX_SYMBOL_SIZE) && (idx < sz)) {
                    state = (state << 8) | uint64(_buffer[idx] & kanzi::byte(0xFF));
                    idx++;
                    nbBits = (idx == sz) ? szBits : nbBits + 8;

                    // 'bits' may overshoot when idx == sz due to padding state bits
                    // It is necessary to compute proper _table indexes
                    // and has no consequence (except bits != 0 at end of chunk)
                    bits += 8;
                }

                // Sanity check
                if (bits > 64)
                    return n;

                uint16 val;

                if (bits >= DECODING_BATCH_SIZE)
                    val = _table[(state >> (bits - DECODING_BATCH_SIZE)) & TABLE_MASK];
                else
                    val = _table[(state << (DECODING_BATCH_SIZE - bits)) & TABLE_MASK];

                bits -= uint8(val);
                block[n++] = kanzi::byte(val >> 8);
            }
        }

        startChunk = endChunk;
    }

    return count;
}
