// Cipher/AES/MyAES.h

#pragma once

#ifndef __CIPHER_MYAES_H
#define __CIPHER_MYAES_H

#include "Common/Types.h"

#include "Interface/ICoder.h"
#include "Stream/InByte.h"
#include "Stream/OutByte.h"

#include "../Common/CipherInterface.h"

// #include "Alien/Crypto/CryptoPP/algparam.h"
// #include "Alien/Crypto/CryptoPP/modes.h"
// #include "Alien/Crypto/CryptoPP/aes.h"

/*
// {23170F69-40C1-278B-0601-000000000100}
DEFINE_GUID(CLSID_CCrypto_AES_Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-0601-000000000000}
DEFINE_GUID(CLSID_CCrypto_AES_Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
*/

#define MyClassCrypto3(Name)  \
class C ## Name:  \
  public ICompressCoder2, \
  public CComObjectRoot, \
public CComCoClass<C ## Name, & CLSID_CCrypto ## Name> { \
  NStream::CInByte _inByte; \
  NStream::COutByte _outByte; \
public: \
BEGIN_COM_MAP(C ## Name) \
  COM_INTERFACE_ENTRY(ICompressCoder2) \
END_COM_MAP() \
DECLARE_NOT_AGGREGATABLE(C ## Name) \
DECLARE_REGISTRY(C ## Name, TEXT("SevenZip.1"), TEXT("SevenZip"), \
  UINT(0), THREADFLAGS_APARTMENT) \
  STDMETHOD(Code)( \
      ISequentialInStream **inStreams, const UINT64 **inSizes, UINT32 numInStreams, \
      ISequentialOutStream **outStreams, const UINT64 **outSizes, UINT32 numOutStreams, \
      ICompressProgressInfo *progress); \
};

// {23170F69-40C1-278B-0601-000000000000}
#define MyClassCrypto2(Name, id, encodingId)  \
DEFINE_GUID(CLSID_CCrypto ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, id, 0x00, 0x00, 0x00, encodingId, 0x00); \
MyClassCrypto3(Name) \

#define MyClassCrypto(Name, id)  \
MyClassCrypto2(Name ## _Encoder, id, 0x01) \
MyClassCrypto2(Name ## _Decoder, id, 0x00) 

MyClassCrypto(_AES128_CBC, 0x01)
MyClassCrypto(_AES256_CBC, 0x81)

#endif