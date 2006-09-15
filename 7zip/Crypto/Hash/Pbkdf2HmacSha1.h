// Pbkdf2HmacSha1.h
// Password-Based Key Derivation Function (RFC 2898, PKCS #5) based on HMAC-SHA-1

#ifndef __PBKDF2HMACSHA1_H
#define __PBKDF2HMACSHA1_H

#include <stddef.h>
#include "../../../Common/Types.h"

namespace NCrypto {
namespace NSha1 {

void Pbkdf2Hmac(const Byte *pwd, size_t pwdSize, const Byte *salt, size_t saltSize, 
    UInt32 numIterations, Byte *key, size_t keySize);

void Pbkdf2Hmac32(const Byte *pwd, size_t pwdSize, const UInt32 *salt, size_t saltSize, 
    UInt32 numIterations, UInt32 *key, size_t keySize);

}}

#endif
