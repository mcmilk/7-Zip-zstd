// Crypto/Rar20/Encoder.h

#include "StdAfx.h"

#include "windows.h"

#include "MyAES.h"
#include "Windows/Defs.h"
#include "Common/Defs.h"

#include "AES_CBC.h"

extern "C"
{
#include "aesopt.h"
}
class CTabInit
{
public:
  CTabInit()
  {
    gen_tabs();
  }
} g_TabInit;

const int kBlockSize = 16;

static HRESULT Encode(
    NStream::CInByte &inByte,
    NStream::COutByte &outByte,
    ISequentialInStream **inStreams,
    const UINT64 **inSizes,
    UINT32 numInStreams,
    ISequentialOutStream **outStreams,
    const UINT64 **outSizes,
    UINT32 numOutStreams,
    ICompressProgressInfo *progress,
    UINT32 keySize)
{
  try
  {
    if (numInStreams != 3 || numOutStreams != 1)
      return E_INVALIDARG;
    
    BYTE key[32];
    BYTE iv[kBlockSize];
    
    /*
    int i;
    for (i = 0; i < kBlockSize; i++)
      iv[i] = 1;
    for (i = 0; i < keySize; i++)
      key[i] = 2;
    
    RETURN_IF_NOT_S_OK(outStreams[1]->Write(iv, kBlockSize, NULL));
    RETURN_IF_NOT_S_OK(outStreams[2]->Write(key, keySize, NULL));
    */
    UINT32 processedSize;
    RETURN_IF_NOT_S_OK(inStreams[1]->Read(iv, kBlockSize, &processedSize));
    if (processedSize != kBlockSize)
      return E_FAIL;

    RETURN_IF_NOT_S_OK(inStreams[2]->Read(key, keySize, &processedSize));
    if (processedSize != keySize)
      return E_FAIL;
    
    CAES_CBCEncoder encoder;
    encoder.enc_key(key, keySize);
    encoder.Init(iv);
    
    inByte.Init(inStreams[0]);
    outByte.Init(outStreams[0]);

    UINT64 nowPos = 0, posPrev = 0;
    while(true)
    {
      BYTE inBlock[kBlockSize], outBlock[kBlockSize];
      UINT32 numBytes;
      inByte.ReadBytes(inBlock, kBlockSize, numBytes);
      for (int i = numBytes; i < kBlockSize; i++)
        inBlock[i] = 0;
      encoder.ProcessData(outBlock, inBlock);
      outByte.WriteBytes(outBlock, kBlockSize);

      nowPos += numBytes;
      if (progress != NULL && (nowPos - posPrev) > (1 << 18))
      {
        UINT64 outSize = nowPos - numBytes + kBlockSize;
        RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&nowPos, &outSize));
        posPrev = nowPos;
      }
      if (numBytes < kBlockSize)
        break;
    }
    outByte.Flush();
    inByte.ReleaseStream();
    outByte.ReleaseStream();
    return S_OK;
  }
  catch(const NStream::CInByteReadException &exception)
  {
    return exception.Result;
  }
  catch(const NStream::COutByteWriteException &exception)
  {
    return exception.Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}

static HRESULT Decode(
    NStream::CInByte &inByte,
    NStream::COutByte &outByte,
    ISequentialInStream **inStreams,
    const UINT64 **inSizes,
    UINT32 numInStreams,
    ISequentialOutStream **outStreams,
    const UINT64 **outSizes,
    UINT32 numOutStreams,
    ICompressProgressInfo *progress,
    UINT32 keySize)
{
  try
  {
    if (numInStreams != 3 || numOutStreams != 1)
      return E_INVALIDARG;
    BYTE key[32];
    BYTE iv[kBlockSize];
    UINT32 processedSize;
    RETURN_IF_NOT_S_OK(inStreams[1]->Read(iv, kBlockSize, &processedSize));
    if (processedSize != kBlockSize)
      return E_FAIL;

    RETURN_IF_NOT_S_OK(inStreams[2]->Read(key, keySize, &processedSize));
    if (processedSize != keySize)
      return E_FAIL;
    
    CAES_CBCCBCDecoder decoder;
    decoder.dec_key(key, keySize);
    decoder.Init(iv);
    
    inByte.Init(inStreams[0]);
    outByte.Init(outStreams[0]);
    
    const UINT64 *outSize = outSizes[0];
    UINT64 nowPos = 0;
    UINT64 posPrev = 0;
    while(true)
    {
      BYTE inBlock[kBlockSize], outBlock[kBlockSize];
      UINT32 numBytes;
      inByte.ReadBytes(inBlock, kBlockSize, numBytes);
      if (numBytes == 0)
        break;
      decoder.ProcessData(outBlock, inBlock);
      UINT32 numBytesToWrite = kBlockSize;
      if (outSize != 0)
        numBytesToWrite = (UINT32)MyMin((*outSize - nowPos), UINT64(numBytesToWrite));
      outByte.WriteBytes(outBlock, numBytesToWrite);
      nowPos += numBytesToWrite;

      if (progress != NULL && (nowPos - posPrev) > (1 << 18))
      {
        UINT64 inSize = inByte.GetProcessedSize();
        RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&inSize, &nowPos));
        posPrev = nowPos;
      }

      if (outSize != 0)
        if (nowPos >= *outSize)
          break;
    }
    outByte.Flush();
    inByte.ReleaseStream();
    outByte.ReleaseStream();
    return S_OK;
  }
  catch(const NStream::CInByteReadException &exception)
  {
    return exception.Result;
  }
  catch(const NStream::COutByteWriteException &exception)
  {
    return exception.Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}


#define MyClassCryptoImp(Name, keySize) \
STDMETHODIMP C ## Name ## _Encoder::Code( \
  ISequentialInStream **inStreams, const UINT64 **inSizes, UINT32 numInStreams, \
  ISequentialOutStream **outStreams, const UINT64 **outSizes, UINT32 numOutStreams, \
  ICompressProgressInfo *progress) \
{ \
  return Encode(_inByte, _outByte, inStreams, inSizes, numInStreams, \
    outStreams, outSizes, numOutStreams, progress, keySize); \
} \
STDMETHODIMP C ## Name ## _Decoder::Code( \
  ISequentialInStream **inStreams, const UINT64 **inSizes, UINT32 numInStreams, \
  ISequentialOutStream **outStreams, const UINT64 **outSizes, UINT32 numOutStreams, \
  ICompressProgressInfo *progress) \
{ \
  return Decode(_inByte, _outByte, inStreams, inSizes, numInStreams, \
    outStreams, outSizes, numOutStreams, progress, keySize); \
}

MyClassCryptoImp(_AES128_CBC, 16)
MyClassCryptoImp(_AES256_CBC, 32)

