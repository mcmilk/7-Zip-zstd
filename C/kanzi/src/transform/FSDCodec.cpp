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

#include <stdexcept>

#include "FSDCodec.hpp"
#include "../Global.hpp"
#include "../Magic.hpp"

using namespace kanzi;
using namespace std;


const int FSDCodec::MIN_LENGTH = 1024;
const kanzi::byte FSDCodec::ESCAPE_TOKEN = kanzi::byte(255);
const kanzi::byte FSDCodec::DELTA_CODING = kanzi::byte(0);
const kanzi::byte FSDCodec::XOR_CODING = kanzi::byte(1);

const uint8 FSDCodec::ZIGZAG1[256] = {
	   253,   251,   249,   247,   245,   243,   241,   239,
	   237,   235,   233,   231,   229,   227,   225,   223,
	   221,   219,   217,   215,   213,   211,   209,   207,
	   205,   203,   201,   199,   197,   195,   193,   191,
	   189,   187,   185,   183,   181,   179,   177,   175,
	   173,   171,   169,   167,   165,   163,   161,   159,
	   157,   155,   153,   151,   149,   147,   145,   143,
	   141,   139,   137,   135,   133,   131,   129,   127,
	   125,   123,   121,   119,   117,   115,   113,   111,
	   109,   107,   105,   103,   101,    99,    97,    95,
	    93,    91,    89,    87,    85,    83,    81,    79,
	    77,    75,    73,    71,    69,    67,    65,    63,
	    61,    59,    57,    55,    53,    51,    49,    47,
	    45,    43,    41,    39,    37,    35,    33,    31,
	    29,    27,    25,    23,    21,    19,    17,    15,
	    13,    11,     9,     7,     5,     3,     1,     0,
	     2,     4,     6,     8,    10,    12,    14,    16,
	    18,    20,    22,    24,    26,    28,    30,    32,
	    34,    36,    38,    40,    42,    44,    46,    48,
	    50,    52,    54,    56,    58,    60,    62,    64,
	    66,    68,    70,    72,    74,    76,    78,    80,
	    82,    84,    86,    88,    90,    92,    94,    96,
	    98,   100,   102,   104,   106,   108,   110,   112,
	   114,   116,   118,   120,   122,   124,   126,   128,
	   130,   132,   134,   136,   138,   140,   142,   144,
	   146,   148,   150,   152,   154,   156,   158,   160,
	   162,   164,   166,   168,   170,   172,   174,   176,
	   178,   180,   182,   184,   186,   188,   190,   192,
	   194,   196,   198,   200,   202,   204,   206,   208,
	   210,   212,   214,   216,   218,   220,   222,   224,
	   226,   228,   230,   232,   234,   236,   238,   240,
	   242,   244,   246,   248,   250,   252,   254,   255,
};


const int8 FSDCodec::ZIGZAG2[256] = {
             0,    -1,     1,    -2,     2,     -3,    3,    -4,
             4,    -5,     5,    -6,     6,     -7,    7,    -8,
             8,    -9,     9,   -10,    10,    -11,   11,   -12,
            12,   -13,    13,   -14,    14,    -15,   15,   -16,
            16,   -17,    17,   -18,    18,    -19,   19,   -20,
            20,   -21,    21,   -22,    22,    -23,   23,   -24,
            24,   -25,    25,   -26,    26,    -27,   27,   -28,
            28,   -29,    29,   -30,    30,    -31,   31,   -32,
            32,   -33,    33,   -34,    34,    -35,   35,   -36,
            36,   -37,    37,   -38,    38,    -39,   39,   -40,
            40,   -41,    41,   -42,    42,    -43,   43,   -44,
            44,   -45,    45,   -46,    46,    -47,   47,   -48,
            48,   -49,    49,   -50,    50,    -51,   51,   -52,
            52,   -53,    53,   -54,    54,    -55,   55,   -56,
            56,   -57,    57,   -58,    58,    -59,   59,   -60,
            60,   -61,    61,   -62,    62,    -63,   63,   -64,
            64,   -65,    65,   -66,    66,    -67,   67,   -68,
            68,   -69,    69,   -70,    70,    -71,   71,   -72,
            72,   -73,    73,   -74,    74,    -75,   75,   -76,
            76,   -77,    77,   -78,    78,    -79,   79,   -80,
            80,   -81,    81,   -82,    82,    -83,   83,   -84,
            84,   -85,    85,   -86,    86,    -87,   87,   -88,
            88,   -89,    89,   -90,    90,    -91,   91,   -92,
            92,   -93,    93,   -94,    94,    -95,   95,   -96,
            96,   -97,    97,   -98,    98,    -99,   99,  -100,
           100,  -101,   101,  -102,   102,   -103,  103,  -104,
           104,  -105,   105,  -106,   106,   -107,  107,  -108,
           108,  -109,   109,  -110,   110,   -111,  111,  -112,
           112,  -113,   113,  -114,   114,   -115,  115,  -116,
           116,  -117,   117,  -118,   118,   -119,  119,  -120,
           120,  -121,   121,  -122,   122,   -123,  123,  -124,
           124,  -125,   125,  -126,   126,   -127,  127,  -128,
};

bool FSDCodec::forward(SliceArray<kanzi::byte>& input, SliceArray<kanzi::byte>& output, int count)
{
    if (count == 0)
        return true;

    if (!SliceArray<kanzi::byte>::isValid(input))
        throw invalid_argument("FSD codec: Invalid input block");

    if (!SliceArray<kanzi::byte>::isValid(output))
        throw invalid_argument("FSD codec: Invalid output block");

    if (input._array == output._array)
        return false;

    if (output._length < getMaxEncodedLength(count))
        return false;

    // If too small, skip
    if (count < MIN_LENGTH)
        return false;

    if (_pCtx != nullptr) {
        Global::DataType dt = (Global::DataType) _pCtx->getInt("dataType", Global::UNDEFINED);

        if ((dt != Global::UNDEFINED) && (dt != Global::MULTIMEDIA) && (dt != Global::BIN))
            return false;
    }

    const kanzi::byte* src = &input._array[input._index];
    kanzi::byte* dst = &output._array[output._index];
    uint magic = Magic::getType(src);

    // Skip detection except for a few candidate types
    switch (magic) {
        case Magic::BMP_MAGIC:
        case Magic::RIFF_MAGIC:
        case Magic::PBM_MAGIC:
        case Magic::PGM_MAGIC:
        case Magic::PPM_MAGIC:
        case Magic::NO_MAGIC:
           break;
        default:
           return false;
    }

    const int srcEnd = count;
    const int dstEnd = getMaxEncodedLength(count);
    const int count10 = count / 10;
    const int count5 = 2 * count10; // count5=count/5 does not guarantee count5=2*count10 !
    uint histo[7][256];
    memset(&histo[0][0], 0, sizeof(histo));

    // Check several step values on a few sub-blocks (no memory allocation)
    const kanzi::byte* in0 = &src[count5 * 0];
    const kanzi::byte* in1 = &src[count5 * 2];
    const kanzi::byte* in2 = &src[count5 * 4];

    for (int i = count10; i < count5; i++) {
        const kanzi::byte b0 = in0[i];
        histo[0][int(b0)]++;
        histo[1][int(b0 ^ in0[i - 1])]++;
        histo[2][int(b0 ^ in0[i - 2])]++;
        histo[3][int(b0 ^ in0[i - 3])]++;
        histo[4][int(b0 ^ in0[i - 4])]++;
        histo[5][int(b0 ^ in0[i - 8])]++;
        histo[6][int(b0 ^ in0[i - 16])]++;
        const kanzi::byte b1 = in1[i];
        histo[0][int(b1)]++;
        histo[1][int(b1 ^ in1[i - 1])]++;
        histo[2][int(b1 ^ in1[i - 2])]++;
        histo[3][int(b1 ^ in1[i - 3])]++;
        histo[4][int(b1 ^ in1[i - 4])]++;
        histo[5][int(b1 ^ in1[i - 8])]++;
        histo[6][int(b1 ^ in1[i - 16])]++;
        const kanzi::byte b2 = in2[i];
        histo[0][int(b2)]++;
        histo[1][int(b2 ^ in2[i - 1])]++;
        histo[2][int(b2 ^ in2[i - 2])]++;
        histo[3][int(b2 ^ in2[i - 3])]++;
        histo[4][int(b2 ^ in2[i - 4])]++;
        histo[5][int(b2 ^ in2[i - 8])]++;
        histo[6][int(b2 ^ in2[i - 16])]++;
    }

    // Find if entropy is lower post transform
    int minIdx = 0;
    int ent[7];

    for (int i = 0; i < 7; i++) {
        ent[i] = Global::computeFirstOrderEntropy1024(3 * count10, histo[i]);

        if (ent[i] < ent[minIdx])
            minIdx = i;
    }

    // If not better, quick exit
    if (ent[minIdx] >= ent[0]) {
        if (_pCtx != nullptr)
            _pCtx->putInt("dataType", Global::detectSimpleType(3 * count10, histo[0]));

        return false;
    }

    if (_pCtx != nullptr)
       _pCtx->putInt("dataType", Global::MULTIMEDIA);

    const int distances[7] = { 0, 1, 2, 3, 4, 8, 16 };
    const int dist = distances[minIdx];
    int largeDeltas = 0;

    // Detect best coding by sampling for large deltas
    for (int i = 2 * count5; i < 3 * count5; i++) {
        const int delta = int(src[i]) - int(src[i - dist]);

        if ((delta < -127) || (delta > 127))
            largeDeltas++;
    }

    // Delta coding works better for pictures & xor coding better for wav files
    // Select xor coding if large deltas are over 3% (ad-hoc threshold)
    const kanzi::byte mode = (largeDeltas > (count5 >> 5)) ? XOR_CODING : DELTA_CODING;
    dst[0] = mode;
    dst[1] = kanzi::byte(dist);
    int srcIdx = 0;
    int dstIdx = 2;

    // Emit first bytes
    for (int i = 0; i < dist; i++)
        dst[dstIdx++] = src[srcIdx++];

    // Emit modified bytes
    if (mode == DELTA_CODING) {
        while ((srcIdx < srcEnd) && (dstIdx < dstEnd - 1)) {
            const int delta = 127 + int(src[srcIdx]) - int(src[srcIdx - dist]);

            if ((delta >= 0) && (delta < 255)) {
                dst[dstIdx++] = kanzi::byte(ZIGZAG1[delta]); // zigzag encode delta
                srcIdx++;
                continue;
            }

            // Skip delta, encode with escape
            dst[dstIdx++] = ESCAPE_TOKEN;
            dst[dstIdx++] = src[srcIdx] ^ src[srcIdx - dist];
            srcIdx++;
        }
    }
    else { // mode == XOR_CODING
        while (srcIdx < srcEnd) {
            dst[dstIdx++] = src[srcIdx] ^ src[srcIdx - dist];
            srcIdx++;
        }
    }

    if (srcIdx != srcEnd)
        return false;

    // Extra check that the transform makes sense
    memset(&histo[0][0], 0, sizeof(uint) * 256);
    const kanzi::byte* out1 = &dst[count5 * 1];
    const kanzi::byte* out2 = &dst[count5 * 3];

    for (int i = 0; i < count10; i++) {
        histo[0][int(out1[i])]++;
        histo[0][int(out2[i])]++;
    }

    const int entropy = Global::computeFirstOrderEntropy1024(count5, histo[0]);

    if (entropy >= ent[0])
        return false;

    input._index += srcIdx;
    output._index += dstIdx;
    return true; // Allowed to expand
}

bool FSDCodec::inverse(SliceArray<kanzi::byte>& input, SliceArray<kanzi::byte>& output, int count)
{
    if (count == 0)
        return true;

    if (!SliceArray<kanzi::byte>::isValid(input))
        throw invalid_argument("FSD codec: Invalid input block");

    if (!SliceArray<kanzi::byte>::isValid(output))
        throw invalid_argument("FSD codec: Invalid output block");

    if (input._array == output._array)
        return false;

    if (count < 4)
        return false;

    if (input._index + count > input._length)
        return false;

    const int srcEnd = count;
    const int dstEnd = output._length - output._index;
    const kanzi::byte* src = &input._array[input._index];
    kanzi::byte* dst = &output._array[output._index];

    // Retrieve mode & step value
    const kanzi::byte mode = src[0];
    const int dist = int(src[1]);

    // Sanity check
    if ((dist < 1) || ((dist > 4) && (dist != 8) && (dist != 16)))
        return false;

    if ((count < dist + 2) || (dist > dstEnd))
        return false;

    // Emit first bytes
    memcpy(&dst[0], &src[2], size_t(dist));
    int srcIdx = dist + 2;
    int dstIdx = dist;

    // Recover original bytes
    if (mode == DELTA_CODING) {
        while ((srcIdx < srcEnd) && (dstIdx < dstEnd)) {
            if (src[srcIdx] != ESCAPE_TOKEN) {
                dst[dstIdx] = kanzi::byte(int(dst[dstIdx - dist]) + ZIGZAG2[int(src[srcIdx])]);
                srcIdx++;
                dstIdx++;
                continue;
            }

            srcIdx++;

            if (srcIdx == srcEnd)
                return false;

            dst[dstIdx] = src[srcIdx] ^ dst[dstIdx - dist];
            srcIdx++;
            dstIdx++;
        }
    }
    else if (mode == XOR_CODING) {
        while ((srcIdx < srcEnd) && (dstIdx < dstEnd)) {
            dst[dstIdx] = src[srcIdx] ^ dst[dstIdx - dist];
            srcIdx++;
            dstIdx++;
        }
    }
    else {
        // Invalid mode
        return false;
    }

    input._index += srcIdx;
    output._index += dstIdx;
    return srcIdx == srcEnd;
}
