/* Compress/HuffmanEncode.h */

#ifndef __COMPRESS_HUFFMANENCODE_H
#define __COMPRESS_HUFFMANENCODE_H

#include "../../Types.h"

/*
Conditions:
  num <= 1024 = 2 ^ NUM_BITS
  Sum(freqs) < 4M = 2 ^ (32 - NUM_BITS)
  maxLen <= 16 = kMaxLen
  Num_Items(p) >= HUFFMAN_TEMP_SIZE(num)
*/
 
void Huffman_Generate(const UInt32 *freqs, UInt32 *p, Byte *lens, UInt32 num, UInt32 maxLen);

#endif
