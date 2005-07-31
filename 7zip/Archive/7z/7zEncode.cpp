// Encode.cpp

#include "StdAfx.h"

#include "7zEncode.h"
#include "7zSpecStream.h"
#include "7zMethods.h"

#include "../../IPassword.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/InOutTempBuffer.h"
#include "../../Common/StreamObjects.h"
#include "../Common/FilterCoder.h"

#ifdef COMPRESS_COPY
static NArchive::N7z::CMethodID k_Copy = { { 0x0 }, 1 };
#include "../../Compress/Copy/CopyCoder.h"
#endif

#ifdef COMPRESS_LZMA
#include "../../Compress/LZMA/LZMAEncoder.h"
static NArchive::N7z::CMethodID k_LZMA = { { 0x3, 0x1, 0x1 }, 3 };
#endif

#ifdef COMPRESS_PPMD
#include "../../Compress/PPMD/PPMDEncoder.h"
static NArchive::N7z::CMethodID k_PPMD = { { 0x3, 0x4, 0x1 }, 3 };
#endif

#ifdef COMPRESS_BCJ_X86
static NArchive::N7z::CMethodID k_BCJ_X86 = { { 0x3, 0x3, 0x1, 0x3 }, 4 };
#include "../../Compress/Branch/x86.h"
#endif

#ifdef COMPRESS_BCJ2
static NArchive::N7z::CMethodID k_BCJ2 = { { 0x3, 0x3, 0x1, 0x1B }, 4 };
#include "../../Compress/Branch/x86_2.h"
#endif

#ifdef COMPRESS_DEFLATE
#ifndef COMPRESS_DEFLATE_ENCODER
#define COMPRESS_DEFLATE_ENCODER
#endif
#endif

#ifdef COMPRESS_DEFLATE_ENCODER
#include "../../Compress/Deflate/DeflateEncoder.h"
static NArchive::N7z::CMethodID k_Deflate = { { 0x4, 0x1, 0x8 }, 3 };
#endif

#ifdef COMPRESS_BZIP2
#ifndef COMPRESS_BZIP2_ENCODER
#define COMPRESS_BZIP2_ENCODER
#endif
#endif

#ifdef COMPRESS_BZIP2_ENCODER
#include "../../Compress/BZip2/BZip2Encoder.h"
static NArchive::N7z::CMethodID k_BZip2 = { { 0x4, 0x2, 0x2 }, 3 };
#endif

static NArchive::N7z::CMethodID k_AES = { { 0x6, 0xF1, 0x7, 0x1}, 4 };

#ifndef EXCLUDE_COM
static const wchar_t *kCryproMethod = L"7zAES";
/*
// {23170F69-40C1-278B-06F1-070100000100}
DEFINE_GUID(CLSID_CCrypto7zAESEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x07, 0x01, 0x00, 0x00, 0x01, 0x00);
*/
#endif

#ifdef CRYPTO_7ZAES
#include "../../Crypto/7zAES/7zAES.h"
#endif

namespace NArchive {
namespace N7z {

static void ConvertBindInfoToFolderItemInfo(const NCoderMixer2::CBindInfo &bindInfo,
    const CRecordVector<CMethodID> decompressionMethods,
    CFolder &folder)
{
  folder.Coders.Clear();
  // bindInfo.CoderMethodIDs.Clear();
  // folder.OutStreams.Clear();
  folder.PackStreams.Clear();
  folder.BindPairs.Clear();
  int i;
  for (i = 0; i < bindInfo.BindPairs.Size(); i++)
  {
    CBindPair bindPair;
    bindPair.InIndex = bindInfo.BindPairs[i].InIndex;
    bindPair.OutIndex = bindInfo.BindPairs[i].OutIndex;
    folder.BindPairs.Add(bindPair);
  }
  for (i = 0; i < bindInfo.Coders.Size(); i++)
  {
    CCoderInfo coderInfo;
    const NCoderMixer2::CCoderStreamsInfo &coderStreamsInfo = bindInfo.Coders[i];
    coderInfo.NumInStreams = coderStreamsInfo.NumInStreams;
    coderInfo.NumOutStreams = coderStreamsInfo.NumOutStreams;
    
    // coderInfo.MethodID = decompressionMethods[i];
    // if (coderInfo.AltCoders.Size() == 0)
    coderInfo.AltCoders.Add(CAltCoderInfo());
    CAltCoderInfo &altCoderInfo = coderInfo.AltCoders.Front();
    altCoderInfo.MethodID = decompressionMethods[i];

    folder.Coders.Add(coderInfo);
  }
  for (i = 0; i < bindInfo.InStreams.Size(); i++)
    folder.PackStreams.Add(bindInfo.InStreams[i]);
}

HRESULT CEncoder::CreateMixerCoder()
{
  _mixerCoderSpec = new NCoderMixer2::CCoderMixer2MT;
  _mixerCoder = _mixerCoderSpec;
  _mixerCoderSpec->SetBindInfo(_bindInfo);
  for (int i = 0; i < _options.Methods.Size(); i++)
  {
    const CMethodFull &methodFull = _options.Methods[i];
    _codersInfo.Add(CCoderInfo());
    CCoderInfo &encodingInfo = _codersInfo.Back();
    CMyComPtr<ICompressCoder> encoder;
    CMyComPtr<ICompressFilter> filter;
    CMyComPtr<ICompressCoder2> encoder2;
    
    if (methodFull.IsSimpleCoder())
    {
      #ifdef COMPRESS_LZMA
      if (methodFull.MethodID == k_LZMA)
        encoder = new NCompress::NLZMA::CEncoder;
      #endif
      
      #ifdef COMPRESS_PPMD
      if (methodFull.MethodID == k_PPMD)
        encoder = new NCompress::NPPMD::CEncoder;
      #endif
      
      #ifdef COMPRESS_BCJ_X86
      if (methodFull.MethodID == k_BCJ_X86)
        filter = new CBCJ_x86_Encoder;
      #endif
      
      #ifdef COMPRESS_COPY
      if (methodFull.MethodID == k_Copy)
        encoder = new NCompress::CCopyCoder;
      #endif
      
      #ifdef COMPRESS_BZIP2_ENCODER
      if (methodFull.MethodID == k_BZip2)
        encoder = new NCompress::NBZip2::CEncoder;
      #endif
      
      #ifdef COMPRESS_DEFLATE_ENCODER
      if (methodFull.MethodID == k_Deflate)
        encoder = new NCompress::NDeflate::NEncoder::CCOMCoder;
      #endif
      
      #ifdef CRYPTO_7ZAES
      if (methodFull.MethodID == k_AES)
        filter = new NCrypto::NSevenZ::CEncoder;
      #endif

      if (filter)
      {
        CFilterCoder *coderSpec = new CFilterCoder;
        encoder = coderSpec;
        coderSpec->Filter = filter;
      }

      #ifndef EXCLUDE_COM
      if (encoder == 0)
      {
        RINOK(_libraries.CreateCoderSpec(methodFull.FilePath, 
              methodFull.EncoderClassID, &encoder));
      }
      #endif
      
      if (encoder == 0)
        return E_FAIL;
      
    }
    else
    {
      #ifdef COMPRESS_BCJ2
      if (methodFull.MethodID == k_BCJ2)
        encoder2 = new CBCJ2_x86_Encoder;
      #endif
      
      #ifndef EXCLUDE_COM
      if (encoder2 == 0)
      {
        RINOK(_libraries.CreateCoder2(methodFull.FilePath, 
              methodFull.EncoderClassID, &encoder2));
      }
      #else
      
      if (encoder2 == 0)
        return E_FAIL;
      #endif
    }
    
    if (methodFull.CoderProperties.Size() > 0)
    {
      CRecordVector<PROPID> propIDs;
      int numProperties = methodFull.CoderProperties.Size();
      NWindows::NCOM::CPropVariant *values = new NWindows::NCOM::CPropVariant[numProperties];
      try
      {
        for (int i = 0; i < numProperties; i++)
        {
          const CProperty &property = methodFull.CoderProperties[i];
          propIDs.Add(property.PropID);
          values[i] = property.Value;
        }
        CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
        if (methodFull.IsSimpleCoder())
        {
          RINOK(encoder.QueryInterface(IID_ICompressSetCoderProperties, 
            &setCoderProperties));
        }
        else
        {
          RINOK(encoder2.QueryInterface(IID_ICompressSetCoderProperties, 
            &setCoderProperties));
        }
        RINOK(setCoderProperties->SetCoderProperties(&propIDs.Front(), values, numProperties));
      }
      catch(...)
      {
        delete []values;
        throw;
      }
      delete []values;
    }
    
    CMyComPtr<ICompressWriteCoderProperties> writeCoderProperties;
    
    if (methodFull.IsSimpleCoder())
    {
      encoder.QueryInterface(IID_ICompressWriteCoderProperties, 
          &writeCoderProperties);
    }
    else
    {
      encoder2.QueryInterface(IID_ICompressWriteCoderProperties, 
          &writeCoderProperties);
    }
    
    if (writeCoderProperties != NULL)
    {
      CSequentialOutStreamImp *outStreamSpec = new CSequentialOutStreamImp;
      CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
      outStreamSpec->Init();
      writeCoderProperties->WriteCoderProperties(outStream);
      
      size_t size = outStreamSpec->GetSize();
      
      // encodingInfo.Properties.SetCapacity(size);
      if (encodingInfo.AltCoders.Size() == 0)
        encodingInfo.AltCoders.Add(CAltCoderInfo());
      CAltCoderInfo &altCoderInfo = encodingInfo.AltCoders.Front();
      altCoderInfo.Properties.SetCapacity(size);
      
      memmove(altCoderInfo.Properties, outStreamSpec->GetBuffer(), size);
    }
    
    CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
    if (methodFull.IsSimpleCoder())
    {
      encoder.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword);
    }
    else
    {
      encoder2.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword);
    }

    if (cryptoSetPassword)
    {
      CByteBuffer buffer;
      const UInt32 sizeInBytes = _options.Password.Length() * 2;
      buffer.SetCapacity(sizeInBytes);
      for (int i = 0; i < _options.Password.Length(); i++)
      {
        wchar_t c = _options.Password[i];
        ((Byte *)buffer)[i * 2] = (Byte)c;
        ((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
      }
      RINOK(cryptoSetPassword->CryptoSetPassword(
        (const Byte *)buffer, sizeInBytes));
    }

    // public ICompressWriteCoderProperties,
    if (methodFull.IsSimpleCoder())
    {
      _mixerCoderSpec->AddCoder(encoder);
    }
    else
    {
      _mixerCoderSpec->AddCoder2(encoder2);
    }
  }
  return S_OK;
}

HRESULT CEncoder::Encode(ISequentialInStream *inStream,
    const UInt64 *inStreamSize,
    CFolder &folderItem,
    ISequentialOutStream *outStream,
    CRecordVector<UInt64> &packSizes,
    ICompressProgressInfo *compressProgress)
{
  if (_mixerCoderSpec == NULL)
  {
    RINOK(CreateMixerCoder());
  }
  _mixerCoderSpec->ReInit();
  // _mixerCoderSpec->SetCoderInfo(0, NULL, NULL, progress);

  CObjectVector<CInOutTempBuffer> inOutTempBuffers;
  CObjectVector<CSequentialOutTempBufferImp *> tempBufferSpecs;
  CObjectVector<CMyComPtr<ISequentialOutStream> > tempBuffers;
  int numMethods = _bindInfo.Coders.Size();
  int i;
  for (i = 1; i < _bindInfo.OutStreams.Size(); i++)
  {
    inOutTempBuffers.Add(CInOutTempBuffer());
    inOutTempBuffers.Back().Create();
    inOutTempBuffers.Back().InitWriting();
  }
  for (i = 1; i < _bindInfo.OutStreams.Size(); i++)
  {
    CSequentialOutTempBufferImp *tempBufferSpec = 
        new CSequentialOutTempBufferImp;
    CMyComPtr<ISequentialOutStream> tempBuffer = tempBufferSpec;
    tempBufferSpec->Init(&inOutTempBuffers[i - 1]);
    tempBuffers.Add(tempBuffer);
    tempBufferSpecs.Add(tempBufferSpec);
  }

  for (i = 0; i < numMethods; i++)
    _mixerCoderSpec->SetCoderInfo(i, NULL, NULL);

  if (_bindInfo.InStreams.IsEmpty())
    return E_FAIL;
  UInt32 mainCoderIndex, mainStreamIndex;
  _bindInfo.FindInStream(_bindInfo.InStreams[0], mainCoderIndex, mainStreamIndex);
  _mixerCoderSpec->SetProgressCoderIndex(mainCoderIndex);
  if (inStreamSize != NULL)
  {
    CRecordVector<const UInt64 *> sizePointers;
    for (UInt32 i = 0; i < _bindInfo.Coders[mainCoderIndex].NumInStreams; i++)
      if (i == mainStreamIndex)
        sizePointers.Add(inStreamSize);
      else
        sizePointers.Add(NULL);
    _mixerCoderSpec->SetCoderInfo(mainCoderIndex, &sizePointers.Front(), NULL);
  }

  
  // UInt64 outStreamStartPos;
  // RINOK(stream->Seek(0, STREAM_SEEK_CUR, &outStreamStartPos));
  
  CSequentialInStreamSizeCount2 *inStreamSizeCountSpec = 
      new CSequentialInStreamSizeCount2;
  CMyComPtr<ISequentialInStream> inStreamSizeCount = inStreamSizeCountSpec;
  CSequentialOutStreamSizeCount *outStreamSizeCountSpec = 
      new CSequentialOutStreamSizeCount;
  CMyComPtr<ISequentialOutStream> outStreamSizeCount = outStreamSizeCountSpec;

  inStreamSizeCountSpec->Init(inStream);
  outStreamSizeCountSpec->Init(outStream);

  CRecordVector<ISequentialInStream *> inStreamPointers;
  CRecordVector<ISequentialOutStream *> outStreamPointers;
  inStreamPointers.Add(inStreamSizeCount);
  outStreamPointers.Add(outStreamSizeCount);
  for (i = 1; i < _bindInfo.OutStreams.Size(); i++)
    outStreamPointers.Add(tempBuffers[i - 1]);
  
  RINOK(_mixerCoder->Code(&inStreamPointers.Front(), NULL, 1,
    &outStreamPointers.Front(), NULL, outStreamPointers.Size(), compressProgress));
  
  ConvertBindInfoToFolderItemInfo(_decompressBindInfo, _decompressionMethods,
      folderItem);
  
  packSizes.Add(outStreamSizeCountSpec->GetSize());
  
  for (i = 1; i < _bindInfo.OutStreams.Size(); i++)
  {
    CInOutTempBuffer &inOutTempBuffer = inOutTempBuffers[i - 1];
    inOutTempBuffer.FlushWrite();
    inOutTempBuffer.InitReading();
    inOutTempBuffer.WriteToStream(outStream);
    packSizes.Add(inOutTempBuffer.GetDataSize());
  }
  
  for (i = 0; i < (int)_bindReverseConverter->NumSrcInStreams; i++)
  {
    int binder = _bindInfo.FindBinderForInStream(
        _bindReverseConverter->DestOutToSrcInMap[i]);
    UInt64 streamSize;
    if (binder < 0)
      streamSize = inStreamSizeCountSpec->GetSize();
    else
      streamSize = _mixerCoderSpec->GetWriteProcessedSize(binder);
    folderItem.UnPackSizes.Add(streamSize);
  }
  for (i = numMethods - 1; i >= 0; i--)
  {
    // folderItem.Coders[numMethods - 1 - i].Properties = _codersInfo[i].Properties;
    for (int j = 0; j < _codersInfo[i].AltCoders.Size(); j++)
      folderItem.Coders[numMethods - 1 - i].AltCoders[j].Properties 
          = _codersInfo[i].AltCoders[j].Properties;
  }
  return S_OK;
}


CEncoder::CEncoder(const CCompressionMethodMode &options):
  _bindReverseConverter(0)
{
  if (options.IsEmpty())
    throw 1;

  _options = options;
  _mixerCoderSpec = NULL;

  if (options.Methods.IsEmpty())
  {
    // it has only password method;
    if (!options.PasswordIsDefined)
      throw 1;
    if (!options.Binds.IsEmpty())
      throw 1;
    NCoderMixer2::CCoderStreamsInfo coderStreamsInfo;
    CMethodFull method;
    
    method.NumInStreams = 1;
    method.NumOutStreams = 1;
    coderStreamsInfo.NumInStreams = method.NumOutStreams;
    coderStreamsInfo.NumOutStreams = method.NumInStreams;
    method.MethodID = k_AES;

    
    #ifndef EXCLUDE_COM
    CMethodInfo2 methodInfo;
    if (!GetMethodInfo(kCryproMethod, methodInfo)) 
      throw 2;
    method.FilePath = methodInfo.FilePath;
    method.EncoderClassID = methodInfo.Encoder;
    // method.EncoderClassID = CLSID_CCrypto7zAESEncoder;
    #endif
    
    _options.Methods.Add(method);
    _bindInfo.Coders.Add(coderStreamsInfo);
  
    _bindInfo.InStreams.Add(0);
    _bindInfo.OutStreams.Add(0);
  }
  else
  {

  UInt32 numInStreams = 0, numOutStreams = 0;
  int i;
  for (i = 0; i < options.Methods.Size(); i++)
  {
    const CMethodFull &methodFull = options.Methods[i];
    NCoderMixer2::CCoderStreamsInfo coderStreamsInfo;
    coderStreamsInfo.NumInStreams = methodFull.NumOutStreams;
    coderStreamsInfo.NumOutStreams = methodFull.NumInStreams;
    if (options.Binds.IsEmpty())
    {
      if (i < options.Methods.Size() - 1)
      {
        NCoderMixer2::CBindPair bindPair;
        bindPair.InIndex = numInStreams + coderStreamsInfo.NumInStreams;
        bindPair.OutIndex = numOutStreams;
        _bindInfo.BindPairs.Add(bindPair);
      }
      else
        _bindInfo.OutStreams.Insert(0, numOutStreams);
      for (UInt32 j = 1; j < coderStreamsInfo.NumOutStreams; j++)
        _bindInfo.OutStreams.Add(numOutStreams + j);
    }
    
    numInStreams += coderStreamsInfo.NumInStreams;
    numOutStreams += coderStreamsInfo.NumOutStreams;

    _bindInfo.Coders.Add(coderStreamsInfo);
  }

  if (!options.Binds.IsEmpty())
  {
    for (i = 0; i < options.Binds.Size(); i++)
    {
      NCoderMixer2::CBindPair bindPair;
      const CBind &bind = options.Binds[i];
      bindPair.InIndex = _bindInfo.GetCoderInStreamIndex(bind.InCoder) + bind.InStream;
      bindPair.OutIndex = _bindInfo.GetCoderOutStreamIndex(bind.OutCoder) + bind.OutStream;
      _bindInfo.BindPairs.Add(bindPair);
    }
    for (i = 0; i < (int)numOutStreams; i++)
      if (_bindInfo.FindBinderForOutStream(i) == -1)
        _bindInfo.OutStreams.Add(i);
  }

  for (i = 0; i < (int)numInStreams; i++)
    if (_bindInfo.FindBinderForInStream(i) == -1)
      _bindInfo.InStreams.Add(i);

  if (_bindInfo.InStreams.IsEmpty())
    throw 1; // this is error

  // Make main stream first in list
  int inIndex = _bindInfo.InStreams[0];
  while (true)
  {
    UInt32 coderIndex, coderStreamIndex;
    _bindInfo.FindInStream(inIndex, coderIndex, coderStreamIndex);
    UInt32 outIndex = _bindInfo.GetCoderOutStreamIndex(coderIndex);
    int binder = _bindInfo.FindBinderForOutStream(outIndex);
    if (binder >= 0)
    {
      inIndex = _bindInfo.BindPairs[binder].InIndex;
      continue;
    }
    for (i = 0; i < _bindInfo.OutStreams.Size(); i++)
      if (_bindInfo.OutStreams[i] == outIndex)
      {
        _bindInfo.OutStreams.Delete(i);
        _bindInfo.OutStreams.Insert(0, outIndex);
        break;
      }
    break;
  }

  if (_options.PasswordIsDefined)
  {
    int numCryptoStreams = _bindInfo.OutStreams.Size();

    for (i = 0; i < numCryptoStreams; i++)
    {
      NCoderMixer2::CBindPair bindPair;
      bindPair.InIndex = numInStreams + i;
      bindPair.OutIndex = _bindInfo.OutStreams[i];
      _bindInfo.BindPairs.Add(bindPair);
    }
    _bindInfo.OutStreams.Clear();

    /*
    if (numCryptoStreams == 0)
      numCryptoStreams = 1;
    */

    for (i = 0; i < numCryptoStreams; i++)
    {
      NCoderMixer2::CCoderStreamsInfo coderStreamsInfo;
      CMethodFull method;
      method.NumInStreams = 1;
      method.NumOutStreams = 1;
      coderStreamsInfo.NumInStreams = method.NumOutStreams;
      coderStreamsInfo.NumOutStreams = method.NumInStreams;
      method.MethodID = k_AES;

      #ifndef EXCLUDE_COM
      CMethodInfo2 methodInfo;
      if (!GetMethodInfo(kCryproMethod, methodInfo)) 
        throw 2;
      method.FilePath = methodInfo.FilePath;
      method.EncoderClassID = methodInfo.Encoder;
      // method.EncoderClassID = CLSID_CCrypto7zAESEncoder;
      #endif

      _options.Methods.Add(method);
      _bindInfo.Coders.Add(coderStreamsInfo);
      _bindInfo.OutStreams.Add(numOutStreams + i);
    }
  }

  }

  for (int i = _options.Methods.Size() - 1; i >= 0; i--)
  {
    const CMethodFull &methodFull = _options.Methods[i];
    _decompressionMethods.Add(methodFull.MethodID);
  }

  _bindReverseConverter = new NCoderMixer2::CBindReverseConverter(_bindInfo);
  _bindReverseConverter->CreateReverseBindInfo(_decompressBindInfo);
}

CEncoder::~CEncoder()
{
  delete _bindReverseConverter;
}

}}
