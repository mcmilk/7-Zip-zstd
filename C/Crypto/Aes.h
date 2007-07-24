/* Aes.h */

#ifndef __AES_H
#define __AES_H

#include "../Types.h"

#define AES_BLOCK_SIZE 16

typedef struct _CAes
{
  unsigned numRounds2; /* = numRounds / 2 */
  UInt32 rkey[(14 + 1) * 4];
} CAes;

/* Call AesGenTables one time before other AES functions */
void MY_FAST_CALL AesGenTables(void);

/* keySize = 16 or 24 or 32 */
void MY_FAST_CALL AesSetKeyEncode(CAes *p, const Byte *key, unsigned keySize); 
void MY_FAST_CALL AesSetKeyDecode(CAes *p, const Byte *key, unsigned keySize);

/* 
AesEncode32 and AesDecode32 functions work with little-endian words.
src and dest can contain same address
*/
void MY_FAST_CALL AesEncode32(const UInt32 *src, UInt32 *dest, const UInt32 *w, unsigned numRounds2);
void MY_FAST_CALL AesDecode32(const UInt32 *src, UInt32 *dest, const UInt32 *w, unsigned numRounds2);

typedef struct _CAesCbc
{
  UInt32 prev[4];
  CAes aes;
} CAesCbc;

void MY_FAST_CALL AesCbcInit(CAesCbc *cbc, const Byte *iv); /* iv size is AES_BLOCK_SIZE */
UInt32 MY_FAST_CALL AesCbcDecode(CAesCbc *cbc, Byte *data, UInt32 size);
UInt32 MY_FAST_CALL AesCbcEncode(CAesCbc *cbc, Byte *data, UInt32 size);

#endif
