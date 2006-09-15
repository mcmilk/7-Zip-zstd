// Crypto/AES/MyAES.h

#ifndef __CIPHER_MYAES_H
#define __CIPHER_MYAES_H

#include "Common/Types.h"
#include "Common/MyCom.h"

#include "../../ICoder.h"
#include "AES_CBC.h"

class CAESFilter:
  public ICompressFilter,
  public ICryptoProperties,
  public CMyUnknownImp 
{ 
protected:
  CAES_CBC AES;
  // Byte Key[32];
  // Byte IV[kAESBlockSize];
public:
  MY_UNKNOWN_IMP1(ICryptoProperties)
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  STDMETHOD(SetKey)(const Byte *data, UInt32 size) = 0;
  STDMETHOD(SetInitVector)(const Byte *data, UInt32 size);
  virtual void SubFilter(const Byte *inBlock, Byte *outBlock) = 0;
};

class CAESEncoder: public CAESFilter
{ 
public:
  STDMETHOD(SetKey)(const Byte *data, UInt32 size);
  virtual void SubFilter(const Byte *inBlock, Byte *outBlock);
};

class CAESDecoder: public CAESFilter
{ 
public:
  STDMETHOD(SetKey)(const Byte *data, UInt32 size);
  virtual void SubFilter(const Byte *inBlock, Byte *outBlock);
};

#define MyClassCrypto3E(Name) class C ## Name: public CAESEncoder { };
#define MyClassCrypto3D(Name) class C ## Name: public CAESDecoder { };

// {23170F69-40C1-278B-0601-000000000000}
#define MyClassCrypto2(Name, id, encodingId)  \
DEFINE_GUID(CLSID_CCrypto_ ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, id, 0x00, 0x00, 0x00, encodingId, 0x00); 

#define MyClassCrypto(Name, id)  \
MyClassCrypto2(Name ## _Encoder, id, 0x01) \
MyClassCrypto3E(Name ## _Encoder) \
MyClassCrypto2(Name ## _Decoder, id, 0x00) \
MyClassCrypto3D(Name ## _Decoder) \

MyClassCrypto(AES_CBC, 0xC1)

class CAesEcbFilter:
  public ICompressFilter,
  public ICryptoProperties,
  public CMyUnknownImp 
{ 
protected:
  AESclass AES;
public:
  MY_UNKNOWN_IMP1(ICryptoProperties)
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  STDMETHOD(SetKey)(const Byte *data, UInt32 size) = 0;
  STDMETHOD(SetInitVector)(const Byte *data, UInt32 size);
  virtual void SubFilter(const Byte *inBlock, Byte *outBlock) = 0;
};

class CAesEcbEncoder: public CAesEcbFilter
{ 
public:
  STDMETHOD(SetKey)(const Byte *data, UInt32 size);
  virtual void SubFilter(const Byte *inBlock, Byte *outBlock);
};

class CAesEcbDecoder: public CAesEcbFilter
{ 
public:
  STDMETHOD(SetKey)(const Byte *data, UInt32 size);
  virtual void SubFilter(const Byte *inBlock, Byte *outBlock);
};

#define MyClassCrypto3E_Ecb(Name) class C ## Name: public CAesEcbEncoder { };
#define MyClassCrypto3D_Ecb(Name) class C ## Name: public CAesEcbDecoder { };

// {23170F69-40C1-278B-0601-000000000000}
#define MyClassCrypto2_Ecb(Name, id, encodingId)  \
DEFINE_GUID(CLSID_CCrypto_ ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, id, 0x00, 0x00, 0x00, encodingId, 0x00); 

#define MyClassCrypto_Ecb(Name, id)  \
MyClassCrypto2_Ecb(Name ## _Encoder, id, 0x01) \
MyClassCrypto3E_Ecb(Name ## _Encoder) \
MyClassCrypto2_Ecb(Name ## _Decoder, id, 0x00) \
MyClassCrypto3D_Ecb(Name ## _Decoder) \

MyClassCrypto_Ecb(AES_ECB, 0xC0)

#endif
