// Crypto/ZipCipher.h

#ifndef __CRYPTO_ZIPCIPHER_H
#define __CRYPTO_ZIPCIPHER_H

#include "Common/MyCom.h"
#include "Common/Random.h"
#include "Common/Types.h"

#include "../../ICoder.h"
#include "../../IPassword.h"

#include "ZipCrypto.h"

namespace NCrypto {
namespace NZip {

class CBuffer2
{
protected:
  BYTE *_buffer;
public:
  CBuffer2();
  ~CBuffer2();
};

class CEncoder : 
  public ICompressCoder,
  public ICryptoSetPassword,
  public ICryptoSetCRC,
  public CMyUnknownImp,
  public CBuffer2
{
  CCipher _cipher;
  UINT32 _crc;
public:
  MY_UNKNOWN_IMP2(
      ICryptoSetPassword,
      ICryptoSetCRC
  )

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(CryptoSetPassword)(const BYTE *data, UINT32 size);
  STDMETHOD(CryptoSetCRC)(UINT32 crc);
};


class CDecoder: 
  public ICompressCoder,
  public ICryptoSetPassword,
  public CMyUnknownImp,
  public CBuffer2
{
  CCipher _cipher;
public:

  MY_UNKNOWN_IMP1(ICryptoSetPassword)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(CryptoSetPassword)(const BYTE *data, UINT32 size);
};

}}

#endif