// Cipher/AES/MyAES.h

#pragma once

#ifndef __CIPHER_MYAES_H
#define __CIPHER_MYAES_H

#include "Common/Types.h"
#include "Common/MyCom.h"

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../Common/InBuffer.h"
#include "../../Common/OutBuffer.h"

// #include "Alien/Crypto/CryptoPP/algparam.h"
// #include "Alien/Crypto/CryptoPP/modes.h"
// #include "Alien/Crypto/CryptoPP/aes.h"

#define MyClassCrypto3(Name)  \
class C ## Name:  \
  public ICompressCoder2, \
  public CMyUnknownImp { \
  CInBuffer _inByte; \
  COutBuffer _outByte; \
public: \
  MY_UNKNOWN_IMP \
  STDMETHOD(Code)( \
      ISequentialInStream **inStreams, const UINT64 **inSizes, UINT32 numInStreams, \
      ISequentialOutStream **outStreams, const UINT64 **outSizes, UINT32 numOutStreams, \
      ICompressProgressInfo *progress); \
};

// {23170F69-40C1-278B-0601-000000000000}
#define MyClassCrypto2(Name, id, encodingId)  \
DEFINE_GUID(CLSID_CCrypto_ ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, id, 0x00, 0x00, 0x00, encodingId, 0x00); \
MyClassCrypto3(Name) \

#define MyClassCrypto(Name, id)  \
MyClassCrypto2(Name ## _Encoder, id, 0x01) \
MyClassCrypto2(Name ## _Decoder, id, 0x00) 

MyClassCrypto(AES128_CBC, 0x01)
MyClassCrypto(AES256_CBC, 0x81)

#endif