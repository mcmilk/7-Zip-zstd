// Crypto/Rar20/Crypto.h

#pragma once

#ifndef __CRYPTO_RAR20_CRYPTO_H
#define __CRYPTO_RAR20_CRYPTO_H

namespace NCrypto {
namespace NRar20 {

class CData
{
  BYTE SubstTable[256];
  UINT32 Keys[4];
  UINT32 SubstLong(UINT32 t)
  {
    return (UINT32)SubstTable[(int)t & 255] | 
           ((UINT32)SubstTable[(int)(t >> 8) & 255] << 8) |
           ((UINT32)SubstTable[(int)(t >> 16) & 255] << 16) | 
           ((UINT32)SubstTable[(int)(t >> 24) & 255] << 24);
  }

  void UpdateKeys(const BYTE *data);
public:
  void EncryptBlock(BYTE *Buf);
  void DecryptBlock(BYTE *Buf);
  void SetPassword(const BYTE *password, UINT32 passwordLength);
};

}}

#endif
