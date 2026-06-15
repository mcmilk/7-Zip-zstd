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
#include <stdexcept>
#include "FPAQDecoder.hpp"
#include "EntropyUtils.hpp"

using namespace kanzi;
using namespace std;


const uint64 FPAQDecoder::TOP = 0x00FFFFFFFFFFFFFF;
const uint64 FPAQDecoder::MASK_0_56 = 0x00FFFFFFFFFFFFFF;
const uint64 FPAQDecoder::MASK_0_32 = 0x00000000FFFFFFFF;
const uint FPAQDecoder::DEFAULT_CHUNK_SIZE = 4 * 1024 * 1024;
const uint FPAQDecoder::MAX_BLOCK_SIZE = 1 << 30;
const int FPAQDecoder::PSCALE = 65536;


FPAQDecoder::FPAQDecoder(InputBitStream& bitstream)
    : _bitstream(bitstream)
{
    reset();
}

FPAQDecoder::~FPAQDecoder()
{
    _dispose();
}

bool FPAQDecoder::reset()
{
    _low = 0;
    _high = TOP;
    _current = 0;
    _ctx = 1;
    _index = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 256; j++)
            _probs[i][j] = PSCALE >> 1;
    }

    _p = _probs[0];
    return true;
}

int FPAQDecoder::decode(kanzi::byte block[], uint blkptr, uint count)
{
    if (count >= MAX_BLOCK_SIZE)
        throw invalid_argument("Invalid block size parameter (max is 1<<30)");

    uint startChunk = blkptr;
    const uint end = blkptr + count;

    // Read bit array from bitstream and decode chunk
    while (startChunk < end) {
        const uint szBytes = uint(EntropyUtils::readVarInt(_bitstream));

        // Sanity check
        if (szBytes >= 2 * count)
            return 0;

        const size_t bufSize = max(szBytes + (szBytes >> 3), 8192u);

        if (_buf.size() < bufSize)
            _buf.resize(bufSize);

        _current = _bitstream.readBits(56);

        if (bufSize > szBytes)
            memset(&_buf[szBytes], 0, bufSize - szBytes);

        _bitstream.readBits(&_buf[0], 8 * szBytes);
        _index = 0;
        const uint chunkSize = min(DEFAULT_CHUNK_SIZE, end - startChunk);
        const uint endChunk = startChunk + chunkSize;
        _p = _probs[0];

        for (uint i = startChunk; i < endChunk; i++) {
            _ctx = 1;
            decodeBit(_p[_ctx]);
            decodeBit(_p[_ctx]);
            decodeBit(_p[_ctx]);
            decodeBit(_p[_ctx]);
            decodeBit(_p[_ctx]);
            decodeBit(_p[_ctx]);
            decodeBit(_p[_ctx]);
            decodeBit(_p[_ctx]);
            block[i] = kanzi::byte(_ctx);
            _p = _probs[(_ctx & 0xFF) >> 6];
        }

        startChunk = endChunk;
    }

    return count;
}


