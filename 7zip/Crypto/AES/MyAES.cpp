// Crypto/AES/MyAES.cpp

#include "StdAfx.h"

#include "windows.h"

#include "MyAES.h"
#include "Windows/Defs.h"

#include "AES_CBC.h"

static const int kAESBlockSize = 16;

extern "C" 
{ 
#include "aesopt.h" 
}

class CTabInit
{
public:
  CTabInit() { gen_tabs();}
} g_TabInit;

STDMETHODIMP CAESFilter::Init()
{
  return S_OK;
}

STDMETHODIMP_(UInt32) CAESFilter::Filter(Byte *data, UInt32 size)
{
  if (size > 0 && size < kAESBlockSize)
    return kAESBlockSize;
  UInt32 i;
  for (i = 0; i + kAESBlockSize <= size; i += kAESBlockSize)
  {
    Byte outBlock[kAESBlockSize];
    SubFilter(data + i, outBlock);
    for (int j = 0; j < kAESBlockSize; j++)
      data[i + j] = outBlock[j];
  }
  return i;
}

STDMETHODIMP CAESFilter::SetInitVector(const Byte *data, UInt32 size)
{
  if (size != 16)
    return E_INVALIDARG;
  AES.Init(data);
  return S_OK;
}

STDMETHODIMP CAESEncoder::SetKey(const Byte *data, UInt32 size)
{
  if (AES.enc_key(data, size) != aes_good)
    return E_FAIL;
  return S_OK;
}

void CAESEncoder::SubFilter(const Byte *inBlock, Byte *outBlock)
{
  AES.Encode(inBlock, outBlock);
}

STDMETHODIMP CAESDecoder::SetKey(const Byte *data, UInt32 size)
{
  if (AES.dec_key(data, size) != aes_good)
    return E_FAIL;
  return S_OK;
}

void CAESDecoder::SubFilter(const Byte *inBlock, Byte *outBlock)
{
  AES.Decode(inBlock, outBlock);
}


/*
  try
  {
    if (numInStreams != 3 || numOutStreams != 1)
      return E_INVALIDARG;
    
    Byte key[32];
    Byte iv[kBlockSize];
    
    /*
    int i;
    for (i = 0; i < kBlockSize; i++)
      iv[i] = 1;
    for (i = 0; i < keySize; i++)
      key[i] = 2;
    
    RINOK(outStreams[1]->Write(iv, kBlockSize, NULL));
    RINOK(outStreams[2]->Write(key, keySize, NULL));
    */
/*
    UInt32 processedSize;
    RINOK(inStreams[1]->Read(iv, kBlockSize, &processedSize));
    if (processedSize != kBlockSize)
      return E_FAIL;

    RINOK(inStreams[2]->Read(key, keySize, &processedSize));
    if (processedSize != keySize)
      return E_FAIL;
    
    CAES_CBCEncoder encoder;
    encoder.enc_key(key, keySize);
    encoder.Init(iv);


static HRESULT Decode(
    CInBuffer &inBuffer,
    COutBuffer &outBuffer,
    ISequentialInStream **inStreams,
    const UInt64 **inSizes,
    UInt32 numInStreams,
    ISequentialOutStream **outStreams,
    const UInt64 **outSizes,
    UInt32 numOutStreams,
    ICompressProgressInfo *progress,
    UInt32 keySize)
{
  try
  {
    if (numInStreams != 3 || numOutStreams != 1)
      return E_INVALIDARG;
    Byte key[32];
    Byte iv[kBlockSize];
    UInt32 processedSize;
    RINOK(inStreams[1]->Read(iv, kBlockSize, &processedSize));
    if (processedSize != kBlockSize)
      return E_FAIL;

    RINOK(inStreams[2]->Read(key, keySize, &processedSize));
    if (processedSize != keySize)
      return E_FAIL;
    
    CAES_CBCDecoder decoder;
    decoder.dec_key(key, keySize);
    decoder.Init(iv);
    
    if (!inBuffer.Create(1 << 20))
      return E_OUTOFMEMORY;
    if (!outBuffer.Create(1 << 20))
      return E_OUTOFMEMORY;
    inBuffer.SetStream(inStreams[0]);
    inBuffer.Init();
    outBuffer.SetStream(outStreams[0]);
    outBuffer.Init();
    
    const UInt64 *outSize = outSizes[0];
    UInt64 nowPos = 0;
    UInt64 posPrev = 0;
    while(true)
    {
      Byte inBlock[kBlockSize], outBlock[kBlockSize];
      UInt32 numBytes;
      inBuffer.ReadBytes(inBlock, kBlockSize, numBytes);
      if (numBytes == 0)
        break;
      decoder.ProcessData(outBlock, inBlock);
      UInt32 numBytesToWrite = kBlockSize;
      if (outSize != 0)
        numBytesToWrite = (UInt32)MyMin((*outSize - nowPos), UInt64(numBytesToWrite));
      outBuffer.WriteBytes(outBlock, numBytesToWrite);
      nowPos += numBytesToWrite;

      if (progress != NULL && (nowPos - posPrev) > (1 << 18))
      {
        UInt64 inSize = inBuffer.GetProcessedSize();
        RINOK(progress->SetRatioInfo(&inSize, &nowPos));
        posPrev = nowPos;
      }

      if (outSize != 0)
        if (nowPos >= *outSize)
          break;
    }
    return outBuffer.Flush();
    // inBuffer.ReleaseStream();
    // outBuffer.ReleaseStream();
    // return S_OK;
  }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return E_FAIL; }
}
*/

