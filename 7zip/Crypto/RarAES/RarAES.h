// Crypto/CRarAES/RarAES.h

#ifndef __CRYPTO_RARAES_H
#define __CRYPTO_RARAES_H

#include "Common/MyCom.h"
#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../Archive/Common/CoderLoader.h"

#include "Common/Types.h"
#include "Common/Buffer.h"

namespace NCrypto {
namespace NRar29 {

class CDecoder: 
  public ICompressFilter,
  public ICompressSetDecoderProperties2,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  Byte _salt[8];
  bool _thereIsSalt;
  CByteBuffer buffer;
  Byte aesKey[16];
  Byte aesInit[16];
  bool _needCalculate;

  CCoderLibrary _aesLib;
  CMyComPtr<ICompressFilter> _aesFilter;

  void Calculate();
  HRESULT CreateFilter();

public:

  MY_UNKNOWN_IMP2(
    ICryptoSetPassword,
    ICompressSetDecoderProperties2)

  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);

  STDMETHOD(CryptoSetPassword)(const Byte *aData, UInt32 aSize);

  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);

  CDecoder();
};

}}

#endif