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

#pragma once
#ifndef knz_Memory
#define knz_Memory

#include <cstring>
#include "types.hpp"


namespace kanzi {

// Prefetch helpers

static KANZI_ALWAYS_INLINE void prefetchRead(const void* ptr) {
#if defined(__GNUG__) || defined(__clang__)
    __builtin_prefetch(ptr, 0, 1);
#elif defined(__x86_64__) || defined(_M_AMD64)
    _mm_prefetch((const char*)ptr, _MM_HINT_T0);
#elif defined(_M_ARM)
    __prefetch(ptr);
#elif defined(_M_ARM64)
    __prefetch2(ptr, 1);
#endif
}

static KANZI_ALWAYS_INLINE void prefetchWrite(const void* ptr) {
#if defined(__GNUG__) || defined(__clang__)
    __builtin_prefetch(ptr, 1, 1);
#elif defined(__x86_64__) || defined(_M_AMD64)
    _mm_prefetch((const char*)ptr, _MM_HINT_T0);
#elif defined(_M_ARM)
    __prefetchw(ptr);
#elif defined(_M_ARM64)
    __prefetch2(ptr, 17);
#endif
}

// Byte-swap helpers

static KANZI_ALWAYS_INLINE uint16 knz_bswap16(uint16 x) {
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 5)
    return __builtin_bswap16(x);
#elif defined(_MSC_VER)
    return _byteswap_ushort(x);
#else
    return (uint16)((x >> 8) | (x << 8));
#endif
}

static KANZI_ALWAYS_INLINE uint32 knz_bswap32(uint32 x) {
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 5)
    return __builtin_bswap32(x);
#elif defined(_MSC_VER)
    return _byteswap_ulong(x);
#else
    return ((x >> 24) |
           ((x >> 8) & 0xFF00) |
           ((x << 8) & 0xFF0000) |
           (x << 24));
#endif
}

static KANZI_ALWAYS_INLINE uint64 knz_bswap64(uint64 x) {
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 5)
    return __builtin_bswap64(x);
#elif defined(_MSC_VER)
    return _byteswap_uint64(x);
#else
    x = ((x & 0xFFFFFFFF00000000ull) >> 32) |
        ((x & 0xFFFFFFFFull) << 32);
    x = ((x & 0xFFFF0000FFFF0000ull) >> 16) |
        ((x & 0xFFFF0000FFFFull) << 16);
    x = ((x & 0xFF00FF00FF00FF00ull) >> 8) |
        ((x & 0xFF00FF00FF00FFull) << 8);
    return x;
#endif
}

#ifdef AGGRESSIVE_OPTIMIZATION
    // There be dragons!
    // User assumes responsibility for alignment and aliasing constraints.
    #define KANZI_MEM_EQ4(x, y) (*(const uint32*)(x) == *(const uint32*)(y))
    #define KANZI_MEM_EQ8(x, y) (*(const uint64*)(x) == *(const uint64*)(y))
#else
    #define KANZI_MEM_EQ4(x, y) (std::memcmp((x), (y), 4) == 0)
    #define KANZI_MEM_EQ8(x, y) (std::memcmp((x), (y), 8) == 0)
#endif

// Detect host endianness

#ifndef HOST_IS_LITTLE
    #if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || defined(__BIG_ENDIAN__)
        #define HOST_IS_LITTLE 0
    #else
        #define HOST_IS_LITTLE 1
    #endif
#endif


template <typename T, bool SourceIsBigEndian>
static KANZI_ALWAYS_INLINE T readEndian(const byte* p) {
    T val;

#ifdef AGGRESSIVE_OPTIMIZATION
    val = *reinterpret_cast<const T*>(p); // may be unaligned
#else
    memcpy(&val, p, sizeof(T));
#endif

    // Swap if host and source endianness differ
#if HOST_IS_LITTLE
    if (SourceIsBigEndian) {
#else
    if (!SourceIsBigEndian) {
#endif
        if (sizeof(T) == 2)
            val = (T)knz_bswap16((uint16)val);
        else if (sizeof(T) == 4)
            val = (T)knz_bswap32((uint32)val);
        else if (sizeof(T) == 8)
            val = (T)knz_bswap64((uint64)val);
    }

    return val;
}

template <typename T, bool TargetIsBigEndian>
static KANZI_ALWAYS_INLINE void writeEndian(byte* p, T val) {

#if HOST_IS_LITTLE
    if (TargetIsBigEndian) {
#else
    if (!TargetIsBigEndian) {
#endif
        if (sizeof(T) == 2)
            val = (T)knz_bswap16((uint16)val);
        else if (sizeof(T) == 4)
            val = (T)knz_bswap32((uint32)val);
        else if (sizeof(T) == 8)
            val = (T)knz_bswap64((uint64)val);
    }

#ifdef AGGRESSIVE_OPTIMIZATION
    *reinterpret_cast<T*>(p) = val;
#else
    memcpy(p, &val, sizeof(T));
#endif
}


class BigEndian {
public:
    static int64 readLong64(const byte* p) { return readEndian<int64, true>(p); }
    static int32 readInt32(const byte* p)  { return readEndian<int32, true>(p); }
    static int16 readInt16(const byte* p)  { return readEndian<int16, true>(p); }

    static void writeLong64(byte* p, int64 v) { writeEndian<int64, true>(p, v); }
    static void writeInt32(byte* p, int32 v)  { writeEndian<int32, true>(p, v); }
    static void writeInt16(byte* p, int16 v)  { writeEndian<int16, true>(p, v); }
};

class LittleEndian {
public:
    static int64 readLong64(const byte* p) { return readEndian<int64, false>(p); }
    static int32 readInt32(const byte* p)  { return readEndian<int32, false>(p); }
    static int16 readInt16(const byte* p)  { return readEndian<int16, false>(p); }

    static void writeLong64(byte* p, int64 v) { writeEndian<int64, false>(p, v); }
    static void writeInt32(byte* p, int32 v)  { writeEndian<int32, false>(p, v); }
    static void writeInt16(byte* p, int16 v)  { writeEndian<int16, false>(p, v); }
};

} // namespace kanzi
#endif
