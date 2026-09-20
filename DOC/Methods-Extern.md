7-Zip external method IDs, that are not included to 7-Zip
---------------------------------------------------------


History of this document
------------------------

- see https://github.com/mcmilk/7-Zip-zstd/commits/master/DOC/Methods-Extern.md


Overview of defined ID ranges
-----------------------------

ID       | Codec, Author                | 7-Zip Plugin Author
---------|------------------------------|----------------------
F7 0x xx | reserved                     |
F7 10 xx | LZHAM, Rich Geldreich        | Rich Geldreich
F7 11 01 | Zstandard, Yann Collet       | Tino Reichardt
F7 11 02 | Brotli, Google               | Tino Reichardt
F7 11 04 | LZ4, Yann Collet             | Tino Reichardt
F7 11 05 | LZ5, Przemyslaw Skibinski    | Tino Reichardt
F7 11 06 | Lizard, Przemyslaw Skibinski | Tino Reichardt
6F 10 702 | XChaCha20, Daniel J. Bernstein | [fzxx](https://github.com/fzxx) 
6F 10 703 | XChaCha20-Poly1305, Daniel J. Bernstein | [fzxx](https://github.com/fzxx) 
6F 10 704 | AES+XChaCha20-Poly1305, fzxx  | [fzxx](https://github.com/fzxx) 
6F 10 705 | AES+XChaCha20+Ascon, fzxx     | [fzxx](https://github.com/fzxx) 


Range F7 10 xx - LZHAM
-----------------------

Description:
Lossless is a data compression codec with LZMA-like ratios but 1.5x-8x faster
decompression speed.

License:
LZHAM library is provided as open source software using the MIT license.

7-Zip Container Header:
``` C
 Byte _ver;
 Byte _dict_size;
 Byte _level;
 Byte _flags;
 Byte _reserved[1];
```

Algorithm author: Rich Geldreich
- Homepage: https://github.com/richgel999/lzham_codec
- Source:   https://github.com/richgel999/lzham_codec

Codec plugin author: Rich Geldreich
- Homepage: http://richg42.blogspot.com/2015/11/lzham-custom-codec-plugin-for-7-zip.html
- Source:   https://github.com/richgel999/lzham_codec_devel


Range F7 11 01, Zstandard
-------------------------

Description:
Zstandard is a real-time compression algorithm, providing high compression
ratios. It offers a very wide range of compression / speed trade-off, while
being backed by a very fast decoder.

License:
Zstandard library is provided as open source software using the BSD license.

7-Zip Container Header:
This header is mandatory and must be exact 3 or 5 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _ver_major;   // currently 1
 Byte _ver_minor;   // currently 2
 Byte _level;       // currently 1..22 or 33..MaxFastLevel
 Byte _reserved[2]; // not given in 3 byte header
```
- this header holds some information about the version, which was
  used for creating that 7-Zip container data
- _ver_major should contain the major release of zstd
- _ver_minor should contain the major release of zstd
- _level should contain the level, the data is packed with
- negative fast level values are stored as fastlevel+32 (33 = fastlevel 1 and so on..)
- the other two bytes, if present, should be set to zero currently and are reserved for future use

Algorithm author: Yann Collet
- Homepage: https://facebook.github.io/zstd/
- Source:   https://github.com/facebook/zstd

Codec plugin author: Tino Reichardt
- Homepage: https://mcmilk.de/projects/7-Zip-zstd/
- Source:   https://github.com/mcmilk/7-Zip-zstd

Modes:
- threading is supported through skippable frame id 0x184D2A50U
- all levels are supported (1..22)
- future versions of zstd will support multithreading out of the box
  - this new format is fully compatible with the one within this codec
- the codec is used as archiv handler also, see ZstdHandler.cpp
  - this handler is does not use any additional headers, it supports the plain
    zstd v1.0 format
  - when compiled with ZSTD_LEGACY_SUPPORT, then support is increased to these
    addtional version numbers of zstd: v0.1 up v0.7
- future formats of this algorithm will be fully compatible with release v1.0
  of Zstandard (Zstandard 0.8 == Zstandard 1.0)
- one ID should be okay for this codec

Versions:
The 7-Zip codec will be kept in sync with the current releases of Zstandard.


Range F7 11 02, Brotli
----------------------

Description:
Brotli is a generic-purpose lossless compression algorithm that compresses
data using a combination of a modern variant of the LZ77 algorithm, Huffman
coding and 2nd order context modeling, with a compression ratio comparable to
the best currently available general-purpose compression methods. It is
similar in speed with deflate but offers more dense compression.

License:
The Brotli library is provided as open source software using the MIT license.

7-Zip Container Header (3 bytes):
This header is mandatory and must be exact 3 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _ver_major; // currently 0
 Byte _ver_minor; // currently 6
 Byte _level;     // currently 0..11 (Brotli quality)
```
- this header holds some information about the version, which was
  used for creating that 7-Zip container data
- _ver_major should contain the major release of brotli
- _ver_minor should contain the major release of brotli
- _level should contain the level, the data is packed with

Algorithm author: Google Staff
- Homepage: https://brotli.org/
- Source:   https://github.com/google/brotli

Codec plugin author: Tino Reichardt
- Homepage: https://mcmilk.de/projects/7-Zip-zstd/
- Source:   https://github.com/mcmilk/7-Zip-zstd

Modes:
- threading is supported through skippable frame id 0x184D2A50U
- all levels are supported (1..11)
- one ID should be okay for this codec

Versions:
The 7-Zip codec will be kept in sync with the current releases of Brotli.


Range F7 11 04, LZ4
-------------------

Description:
LZ4 is lossless compression algorithm, providing compression speed at 400 MB/s
per core (0.16 Bytes/cycle). It features an extremely fast decoder, with speed
in multiple GB/s per core (0.71 Bytes/cycle). A high compression derivative,
called LZ4_HC, is available, trading customizable CPU time for compression
ratio.

License:
LZ4 library is provided as open source software using the BSD license.

7-Zip Container Header:
This header is mandatory and must be exact 3 or 5 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _ver_major;   // currently 1
 Byte _ver_minor;   // currently 7
 Byte _level;       // 1..12
 Byte _reserved[2]; // not given in 3 byte header
```
- this header holds some information about the version, which was
  used for creating that 7-Zip container data
- _ver_major should contain the major release of LZ4
- _ver_minor should contain the major release of LZ4
- _level should contain the level, the data is packed with
- the other two bytes, if present, should be set to zero currently and are reserved for future use

Algorithm author: Yann Collet
- Homepage: https://lz4.github.io/lz4/
- Source:   https://github.com/lz4/lz4

Codec plugin author: Tino Reichardt
- Homepage: https://mcmilk.de/projects/7-Zip-zstd/
- Source:   https://github.com/mcmilk/7-Zip-zstd

Modes:
- threading is supported through skippable frame id 0x184D2A50U
- all levels of LZ4 v1.7.4 are supported (1..16)
- the codec is used as archiv handler also, see Lz4Handler.cpp
  - this handler is does not use any additional headers
  - it supports the plain LZ4 v1.7.4 format (this covers all current lz4
    implemetations)
- future formats of this algorithm should be fully compatible with current
  release
- one ID should be okay for this codec

Versions:
The 7-Zip codec will be kept in sync with the current releases of LZ4.


Range F7 11 05, LZ5
-------------------

Description:
LZ5 is a modification of LZ4 which gives a better ratio at cost of slower
compression and decompression.

License:
LZ5 library is provided as open source software using the BSD license.

7-Zip Container Header:
This header is mandatory and must be exact 3 or 5 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _ver_major;   // currently 1
 Byte _ver_minor;   // currently 5
 Byte _level;       // 1..15
 Byte _reserved[2]; // not given in 3 byte header
```
- this header holds some information about the version, which was used for
  creating that 7-Zip container data
- _ver_major should contain the major release of LZ5
- _ver_minor should contain the major release of LZ5
- _level should contain the level, the data is packed with
- the other two bytes, if present, should be set to zero currently and are reserved for future use

Algorithm author: Przemyslaw Skibinski
- Homepage: https://github.com/inikep/lz5
- Source:   https://github.com/inikep/lz5

Codec plugin author: Tino Reichardt
- Homepage: https://mcmilk.de/projects/7-Zip-zstd/
- Source:   https://github.com/mcmilk/7-Zip-zstd

Modes:
- threading is supported through skippable frame id 0x184D2A50U
- all levels of v1.5 are supported, which means: 1..15
- the codec is used as archiv handler also, see Lz5Handler.cpp
  - this handler is does not use any additional headers, it supports the plain
    lz5 v1.5 format
- future formats of this algorithm will not follow
- one ID should be okay for this codec

Versions:
The 7-Zip LZ5 codec will be frozen to v1.5 of this codec. No updates are planned.

Range F7 11 06, Lizard
----------------------

Description:
Lizard is an efficient compressor with very fast decompression. It achieves
compression ratio that is comparable to zip/zlib and zstd/brotli (at low and
medium compression levels) at decompression speed of 1000 MB/s and faster.

License:
Lizard library is provided as open source software using the BSD license.

7-Zip Container Header:
This header is mandatory and must be exact 3 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _ver_major;  // currently 1
 Byte _ver_minor;  // currently 0
 Byte _level;      // 10..49
```
- this header holds some information about the version, which was used for
  creating that 7-Zip container data
- _ver_major should contain the major release of Lizard
- _ver_minor should contain the major release of Lizard
- _level should contain the level, the data is packed with

Algorithm author: Przemyslaw Skibinski
- Homepage: https://github.com/inikep/lizard
- Source:   https://github.com/inikep/lizard

Codec plugin author: Tino Reichardt
- Homepage: https://mcmilk.de/projects/7-Zip-zstd/
- Source:   https://github.com/mcmilk/7-Zip-zstd

Modes:
- threading is supported through skippable frame id 0x184D2A50U
- all compression levels of v1.0 are supported, which means:
  1. Level 10..19
   - fastLZ4, designed to give better decompression speed than LZ4 i.e. over 2000 MB/s
  2. Level 20..29
   - LIZv1, designed to give better ratio than LZ4 keeping 75% decompression speed
  3. Level 30..39
   - fastLZ4 + Huffman, add Huffman coding to fastLZ4
  4. Level 40..49
   - LIZv1 + Huffman, give the best ratio
   - comparable to zlib and low levels of zstd/brotli
   - but also decompression speed of 1000 MB/s
- the codec is used as archiv handler also, see LizardHandler.cpp
  - this handler is does not use any additional headers, it supports the plain lizarv v1.0 format
- future formats of this algorithm will not follow
- one ID should be okay for this codec

Versions:
The 7-Zip Lizard codec will be kept in sync with the current releases of Lizard.


Range 6F 10 702, XChaCha20
---------------------------

Description:
XChaCha20 is an extended-nonce variant of the ChaCha20 stream cipher, designed by
Daniel J. Bernstein. It uses a 256-bit key and a 192-bit nonce, providing a larger
nonce space than the original ChaCha20 (which uses a 96-bit nonce). This makes it
safe to generate nonces randomly without risking nonce reuse. XChaCha20 is defined
in RFC 8439 (formerly draft-irtf-cfrg-xchacha).

License:
The XChaCha20 implementation is provided as open source software using the
GNU LGPL v2.1+ license.

7-Zip Container Header:
This header is mandatory and must be exact 2 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _derivation_mode;  // 0 = SHA-256 iterative
 Byte _num_cycles_power; // key derivation iterations = 2^value
```
- _derivation_mode specifies the key derivation function used
  - 0: SHA-256 iterative hashing (same as 7zAES)
- _num_cycles_power specifies the number of iterations for key derivation
  - the actual iteration count is 2^_num_cycles_power

Encryption Integrator: fzxx
- Homepage: https://github.com/fzxx
- Source:   https://github.com/fzxx/7-Zip-zstd-crypto

Modes:
- key derivation uses SHA-256 iterative hashing, compatible with 7zAES
- no authentication (encryption only)
- SIMD acceleration: SSE2, AVX2, ARM NEON
- one ID should be okay for this codec


Range 6F 10 703, XChaCha20-Poly1305
------------------------------------

Description:
XChaCha20-Poly1305 is an Authenticated Encryption with Associated Data (AEAD)
cipher combining the XChaCha20 stream cipher with the Poly1305 message
authentication code. It provides both confidentiality and data integrity
verification. The Poly1305 MAC generates a 16-byte authentication tag that
is verified during decryption to ensure the data has not been tampered with.

License:
The XChaCha20-Poly1305 implementation is provided as open source software using
the GNU LGPL v2.1+ license.

7-Zip Container Header:
This header is mandatory and must be exact 2 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _derivation_mode;  // 0 = SHA-256 iterative
 Byte _num_cycles_power; // key derivation iterations = 2^value
```
- _derivation_mode specifies the key derivation function used
  - 0: SHA-256 iterative hashing (same as 7zAES)
- _num_cycles_power specifies the number of iterations for key derivation
  - the actual iteration count is 2^_num_cycles_power

Encryption Integrator: fzxx
- Homepage: https://github.com/fzxx
- Source:   https://github.com/fzxx/7-Zip-zstd-crypto

Modes:
- key derivation uses SHA-256 iterative hashing, compatible with 7zAES
- authentication via Poly1305 MAC (16-byte tag)
- decryption verifies the authentication tag and reports error on mismatch
- SIMD acceleration: SSE2, AVX2, ARM NEON
- one ID should be okay for this codec


Range 6F 10 704, AES+XChaCha20-Poly1305 (AXP)
----------------------------------------------

Description:
AES+XChaCha20-Poly1305 (AXP) is a cascade cipher that applies AES-256-CTR and
XChaCha20 sequentially, with Poly1305 providing authentication. The cascade
construction uses three independent subkeys derived from the user's password:
a 32-byte AES key, a 32-byte XChaCha20 key, and a 32-byte Poly1305 one-time
key (derived from XChaCha20). The 96-byte cascade key is derived using
PBKDF2-HMAC-SHA512 and then split into individual subkeys via HKDF-BLAKE2sp.

License:
The AES+XChaCha20-Poly1305 implementation is provided as open source software
using the GNU LGPL v2.1+ license.

7-Zip Container Header:
This header is mandatory and must be exact 2 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _derivation_mode;  // 1 = PBKDF2-HMAC-SHA512 + HKDF-BLAKE2sp
 Byte _num_cycles_power; // key derivation iterations = 2^value
```
- _derivation_mode specifies the key derivation function used
  - 1: PBKDF2-HMAC-SHA512 + HKDF-BLAKE2sp (cascade mode)
- _num_cycles_power specifies the number of iterations for key derivation
  - the actual iteration count is 2^_num_cycles_power

Algorithm author: fzxx (cascade design)
- AES-256: NIST FIPS-197
- XChaCha20: Daniel J. Bernstein
- Poly1305: Daniel J. Bernstein
- PBKDF2: RSA Laboratories (RFC 8018)
- HKDF: Hugo Krawczyk (RFC 5869)

Encryption Integrator: fzxx
- Homepage: https://github.com/fzxx
- Source:   https://github.com/fzxx/7-Zip-zstd-crypto

Modes:
- key derivation uses PBKDF2-HMAC-SHA512 + HKDF-BLAKE2sp (96-byte cascade key)
- the cascade key is split into three 32-byte subkeys for AES, XChaCha20, and Poly1305
- encryption: data is first encrypted with AES-256-CTR, then with XChaCha20
- authentication via Poly1305 MAC (16-byte tag)
- decryption verifies the authentication tag and reports error on mismatch
- AES hardware acceleration (AES-NI) is used when available
- XChaCha20 SIMD acceleration: SSE2, AVX2, ARM NEON
- one ID should be okay for this codec


Range 6F 10 705, AES+XChaCha20+Ascon (AXA)
-------------------------------------------

Description:
AES+XChaCha20+Ascon (AXA) is a cascade cipher that applies AES-256-CTR and
XChaCha20 sequentially, with Ascon-128a providing authentication. Ascon is the
winner of the NIST Lightweight Cryptography standardization project (February
2023). It provides lightweight authenticated encryption suitable for constrained
environments. The cascade construction uses three independent subkeys derived
from the user's password: a 32-byte AES key, a 32-byte XChaCha20 key, and a
16-byte Ascon key. The 96-byte cascade key is derived using PBKDF2-HMAC-SHA512
and then split into individual subkeys via HKDF-BLAKE2sp.

License:
The AES+XChaCha20+Ascon implementation is provided as open source software
using the GNU LGPL v2.1+ license.

7-Zip Container Header:
This header is mandatory and must be exact 2 bytes. The data within that header
is for informational purposes only and not used by the decoder. If the header
is not there, or has another size, the decoder will not decompress the content.
``` C
 Byte _derivation_mode;  // 1 = PBKDF2-HMAC-SHA512 + HKDF-BLAKE2sp
 Byte _num_cycles_power; // key derivation iterations = 2^value
```
- _derivation_mode specifies the key derivation function used
  - 1: PBKDF2-HMAC-SHA512 + HKDF-BLAKE2sp (cascade mode)
- _num_cycles_power specifies the number of iterations for key derivation
  - the actual iteration count is 2^_num_cycles_power

Algorithm author: fzxx (cascade design)
- AES-256: NIST FIPS-197
- XChaCha20: Daniel J. Bernstein
- Ascon: Christoph Dobraunig et al. (Graz University of Technology)
- PBKDF2: RSA Laboratories (RFC 8018)
- HKDF: Hugo Krawczyk (RFC 5869)

Encryption Integrator: fzxx
- Homepage: https://github.com/fzxx
- Source:   https://github.com/fzxx/7-Zip-zstd-crypto

Cascade Modes:
- key derivation uses PBKDF2-HMAC-SHA512 + HKDF-BLAKE2sp (96-byte cascade key), subkeys are independently derived from the cascade key via HKDF-BLAKE2sp with unique info strings.
- AES hardware acceleration (AES-NI) is used when available
- XChaCha20 SIMD acceleration: SSE2, AVX2, ARM NEON
- Ascon SIMD acceleration: SSE2, AVX-512, ARM NEON