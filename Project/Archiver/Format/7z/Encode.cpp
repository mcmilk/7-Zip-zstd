// Encode.cpp

#include "StdAfx.h"

#include "Encode.h"

#include "../../../Compress/Interface/CompressInterface.h"

#include "Common/Defs.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"

#include "Windows/Defs.h"

#include "Util/InOutTempBuffer.h"

#include "StreamObjects2.h"

#include "../../../Compress/LZ/MatchFinder/MT/MT.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"


#ifdef COMPRESS_COPY
static NArchive::N7z::CMethodID k_Copy = { { 0x0 }, 1 };
#include "Compression/CopyCoder.h"
#endif

#ifdef COMPRESS_LZMA
#include "../../../Compress/LZ/LZMA/Encoder.h"
static NArchive::N7z::CMethodID k_LZMA = { { 0x3, 0x1, 0x1 }, 3 };
#endif

#ifdef COMPRESS_PPMD
#include "../../../Compress/PPM/PPMD/Encoder.h"
static NArchive::N7z::CMethodID k_PPMD = { { 0x3, 0x4, 0x1 }, 3 };
#endif

#ifdef COMPRESS_BCJ_X86
static NArchive::N7z::CMethodID k_BCJ_X86 = { { 0x3, 0x3, 0x1, 0x3 }, 4 };
#include "../../../Compress/Convert/Branch/x86.h"
#endif

#ifdef COMPRESS_BCJ2
static NArchive::N7z::CMethodID k_BCJ2 = { { 0x3, 0x3, 0x1, 0x1B }, 4 };
#include "../../../Compress/Convert/Branch/x86_2.h"
#endif

#ifdef COMPRESS_DEFLATE
#include "../../../Compress/LZ/Deflate/Encoder.h"
static NArchive::N7z::CMethodID k_Deflate = { { 0x4, 0x1, 0x8 }, 3 };
#endif

#ifdef COMPRESS_BZIP2
#include "../../../Compress/BWT/BZip2/Encoder.h"
static NArchive::N7z::CMethodID k_BZip2 = { { 0x4, 0x2, 0x2 }, 3 };
#endif

static NArchive::N7z::CMethodID k_AES = { { 0x6, 0xF1, 0x7, 0x1}, 4 };

#ifndef EXCLUDE_COM
#include "../../../Crypto/Cipher/7zAES/7zAES.h"
#endif

#ifdef CRYPTO_7ZAES
#include "../../../Crypto/Cipher/7zAES/7zAES.h"
#endif


#ifdef COMPRESS_MF_PAT
#include "../../../Compress/LZ/MatchFinder/Patricia/Pat2.h"
#include "../../../Compress/LZ/MatchFinder/Patricia/Pat2H.h"
#include "../../../Compress/LZ/MatchFinder/Patricia/Pat3H.h"
#include "../../../Compress/LZ/MatchFinder/Patricia/Pat4H.h"
#include "../../../Compress/LZ/MatchFinder/Patricia/Pat2R.h"
#endif

#ifdef COMPRESS_MF_BT
#include "../../../Compress/LZ/MatchFinder/BinTree/BinTree2.h"
#include "../../../Compress/LZ/MatchFinder/BinTree/BinTree3.h"
#include "../../../Compress/LZ/MatchFinder/BinTree/BinTree4.h"
#include "../../../Compress/LZ/MatchFinder/BinTree/BinTree4b.h"
#endif

#ifdef COMPRESS_MF_HC
#include "../../../Compress/LZ/MatchFinder/HashChain/HC3.h"
#include "../../../Compress/LZ/MatchFinder/HashChain/HC4.h"
#endif


namespace NArchive {
namespace N7z {

static void ConvertBindInfoToFolderItemInfo(const NCoderMixer2::CBindInfo &bindInfo,
    const CRecordVector<CMethodID> decompressionMethods,
    CFolderItemInfo &folderItemInfo)
{
  folderItemInfo.CodersInfo.Clear();
  // bindInfo.CoderMethodIDs.Clear();
  // folderItemInfo.OutStreams.Clear();
  folderItemInfo.PackStreams.Clear();
  folderItemInfo.BindPairs.Clear();
  for (int i = 0; i < bindInfo.BindPairs.Size(); i++)
  {
    CBindPair bindPair;
    bindPair.InIndex = bindInfo.BindPairs[i].InIndex;
    bindPair.OutIndex = bindInfo.BindPairs[i].OutIndex;
    folderItemInfo.BindPairs.Add(bindPair);
  }
  for (i = 0; i < bindInfo.CodersInfo.Size(); i++)
  {
    CCoderInfo coderInfo;
    const NCoderMixer2::CCoderStreamsInfo &coderStreamsInfo = bindInfo.CodersInfo[i];
    coderInfo.NumInStreams = coderStreamsInfo.NumInStreams;
    coderInfo.NumOutStreams = coderStreamsInfo.NumOutStreams;
    
    // coderInfo.DecompressionMethod = decompressionMethods[i];
    // if (coderInfo.AltCoders.Size() == 0)
    coderInfo.AltCoders.Add(CAltCoderInfo());
    CAltCoderInfo &altCoderInfo = coderInfo.AltCoders.Front();
    altCoderInfo.DecompressionMethod = decompressionMethods[i];

    folderItemInfo.CodersInfo.Add(coderInfo);
  }
  for (i = 0; i < bindInfo.InStreams.Size(); i++)
  {
    CPackStreamInfo packStreamInfo;
    packStreamInfo.Index = bindInfo.InStreams[i];
    folderItemInfo.PackStreams.Add(packStreamInfo);
  }
}

HRESULT CEncoder::CreateMixerCoder()
{
  _mixerCoderSpec = new CComObjectNoLock<NCoderMixer2::CCoderMixer2>;
  _mixerCoder = _mixerCoderSpec;
  _mixerCoderSpec->SetBindInfo(_bindInfo);
  for (int i = 0; i < _options.Methods.Size(); i++)
  {
    const CMethodFull &methodFull = _options.Methods[i];
    _codersInfo.Add(CCoderInfo());
    CCoderInfo &encodingInfo = _codersInfo.Back();
    CComPtr<ICompressCoder> encoder;
    CComPtr<ICompressCoder2> encoder2;
    
    if (methodFull.MethodInfoEx.IsSimpleCoder())
    {
      #ifdef COMPRESS_LZMA
      if (methodFull.MethodInfoEx.MethodID == k_LZMA)
        encoder = new CComObjectNoLock<NCompress::NLZMA::CEncoder>;
      #endif
      
      #ifdef COMPRESS_PPMD
      if (methodFull.MethodInfoEx.MethodID == k_PPMD)
        encoder = new CComObjectNoLock<NCompress::NPPMD::CEncoder>;
      #endif
      
      #ifdef COMPRESS_BCJ_X86
      if (methodFull.MethodInfoEx.MethodID == k_BCJ_X86)
        encoder = new CComObjectNoLock<CBCJ_x86_Encoder>;
      #endif
      
      #ifdef COMPRESS_COPY
      if (methodFull.MethodInfoEx.MethodID == k_Copy)
        encoder = new CComObjectNoLock<NCompression::CCopyCoder>;
      #endif
      
      #ifdef COMPRESS_BZIP2
      if (methodFull.MethodInfoEx.MethodID == k_BZip2)
        encoder = new CComObjectNoLock<NCompress::NBZip2::NEncoder::CCoder>;
      #endif
      
      #ifdef COMPRESS_DEFLATE
      if (methodFull.MethodInfoEx.MethodID == k_Deflate)
        encoder = new CComObjectNoLock<NDeflate::NEncoder::CCOMCoder>;
      #endif
      
      #ifdef CRYPTO_7ZAES
      if (methodFull.MethodInfoEx.MethodID == k_AES)
        encoder = new CComObjectNoLock<NCrypto::NSevenZ::CEncoder>;
      #endif

      #ifndef EXCLUDE_COM
      if (encoder == 0)
      {
        RETURN_IF_NOT_S_OK(encoder.CoCreateInstance(methodFull.EncoderClassID));
      }
      #endif
      
      if (encoder == 0)
        return E_FAIL;
      
    }
    else
    {
      #ifdef COMPRESS_BCJ2
      if (methodFull.MethodInfoEx.MethodID == k_BCJ2)
        encoder2 = new CComObjectNoLock<CBCJ2_x86_Encoder>;
      #endif
      
      #ifndef EXCLUDE_COM
      if (encoder2 == 0)
      {
        RETURN_IF_NOT_S_OK(encoder2.CoCreateInstance(methodFull.EncoderClassID));
      }
      #else
      
      if (encoder2 == 0)
        return E_FAIL;
      #endif
    }
    
    
    if (methodFull.MatchFinderIsDefined)
    {
      CComPtr<IInWindowStreamMatch> matchFinder;
      #ifdef COMPRESS_MF_PAT
      if (methodFull.MatchFinderName.CompareNoCase(_TEXT("Pat2")) == 0)
        matchFinder = new CComObjectNoLock<NPat2::CPatricia>;
      else if (methodFull.MatchFinderName.CompareNoCase(_TEXT("Pat2H")) == 0)
        matchFinder = new CComObjectNoLock<NPat2H::CPatricia>;
      else if (methodFull.MatchFinderName.CompareNoCase(_TEXT("Pat2R")) == 0)
        matchFinder = new CComObjectNoLock<NPat2R::CPatricia>;
      else if (methodFull.MatchFinderName.CompareNoCase(_TEXT("Pat3H")) == 0)
        matchFinder = new CComObjectNoLock<NPat3H::CPatricia>;
      else if (methodFull.MatchFinderName.CompareNoCase(_TEXT("Pat4H")) == 0)
        matchFinder = new CComObjectNoLock<NPat4H::CPatricia>;
      #endif
      
      #ifdef COMPRESS_MF_BT
      if (methodFull.MatchFinderName.CompareNoCase(_TEXT("BT2")) == 0)
        matchFinder = new CComObjectNoLock<NBT2::CMatchFinderBinTree>;
      else if (methodFull.MatchFinderName.CompareNoCase(_TEXT("BT3")) == 0)
        matchFinder = new CComObjectNoLock<NBT3::CMatchFinderBinTree>;
      else if (methodFull.MatchFinderName.CompareNoCase(_TEXT("BT4")) == 0)
        matchFinder = new CComObjectNoLock<NBT4::CMatchFinderBinTree>;
      else if (methodFull.MatchFinderName.CompareNoCase(_TEXT("BT4b")) == 0)
        matchFinder = new CComObjectNoLock<NBT4b::CMatchFinderBinTree>;
      #endif
      
      #ifdef COMPRESS_MF_HC
      if (methodFull.MatchFinderName.CompareNoCase(_TEXT("HC3")) == 0)
        matchFinder = new CComObjectNoLock<NHC3::CMatchFinderHC>;
      else if (methodFull.MatchFinderName.CompareNoCase(_TEXT("HC4")) == 0)
        matchFinder = new CComObjectNoLock<NHC4::CMatchFinderHC>;
      #endif
      
      #ifndef EXCLUDE_COM
      if (matchFinder == 0)
      {
        RETURN_IF_NOT_S_OK(matchFinder.CoCreateInstance(methodFull.MatchFinderClassID));
      }
      #endif
      
      if (matchFinder == 0)
        return E_FAIL;
      
      CComPtr<IInitMatchFinder> initMatchFinder;
      if (methodFull.MethodInfoEx.IsSimpleCoder())
      {
        RETURN_IF_NOT_S_OK(encoder->QueryInterface(&initMatchFinder));
      }
      else
      {
        RETURN_IF_NOT_S_OK(encoder2->QueryInterface(&initMatchFinder));
      }
      
      if (_options.MultiThread)
      {
        CComObjectNoLock<CMatchFinderMT> *matchFinderMTSpec = new 
          CComObjectNoLock<CMatchFinderMT>;
        CComPtr<IInWindowStreamMatch> matchFinderMT = matchFinderMTSpec;
        RETURN_IF_NOT_S_OK(matchFinderMTSpec->SetMatchFinder(matchFinder, 
          _options.MultiThreadMult));
        initMatchFinder->InitMatchFinder(matchFinderMT);
      }
      else
        initMatchFinder->InitMatchFinder(matchFinder);
    }
    
    if (methodFull.EncoderProperties.Size() > 0)
    {
      std::vector<NWindows::NCOM::CPropVariant> properties;
      std::vector<PROPID> propIDs;
      INT32 numProperties = methodFull.EncoderProperties.Size();
      for (int i = 0; i < numProperties; i++)
      {
        const CProperty &property = methodFull.EncoderProperties[i];
        propIDs.push_back(property.PropID);
        properties.push_back(property.Value);
      }
      CComPtr<ICompressSetEncoderProperties2> setEncoderProperties2;
      if (methodFull.MethodInfoEx.IsSimpleCoder())
      {
        RETURN_IF_NOT_S_OK(encoder->QueryInterface(&setEncoderProperties2));
      }
      else
      {
        RETURN_IF_NOT_S_OK(encoder2->QueryInterface(&setEncoderProperties2));
      }
      
      RETURN_IF_NOT_S_OK(setEncoderProperties2->SetEncoderProperties2(&propIDs.front(),
        &properties.front(), numProperties));
    }
    
    if (methodFull.CoderProperties.Size() > 0)
    {
      std::vector<NWindows::NCOM::CPropVariant> properties;
      std::vector<PROPID> propIDs;
      INT32 numProperties = methodFull.CoderProperties.Size();
      for (int i = 0; i < numProperties; i++)
      {
        const CProperty &property = methodFull.CoderProperties[i];
        propIDs.push_back(property.PropID);
        properties.push_back(property.Value);
      }
      CComPtr<ICompressSetCoderProperties2> setCoderProperties2;
      if (methodFull.MethodInfoEx.IsSimpleCoder())
      {
        RETURN_IF_NOT_S_OK(encoder->QueryInterface(&setCoderProperties2));
      }
      else
      {
        RETURN_IF_NOT_S_OK(encoder2->QueryInterface(&setCoderProperties2));
      }
      
      RETURN_IF_NOT_S_OK(setCoderProperties2->SetCoderProperties2(&propIDs.front(),
        &properties.front(), numProperties));
    }
    
    CComPtr<ICompressWriteCoderProperties> writeCoderProperties;
    
    if (methodFull.MethodInfoEx.IsSimpleCoder())
    {
      encoder->QueryInterface(&writeCoderProperties);
    }
    else
    {
      encoder2->QueryInterface(&writeCoderProperties);
    }
    
    if (writeCoderProperties != NULL)
    {
      CComObjectNoLock<CSequentialOutStreamImp> *outStreamSpec = new 
        CComObjectNoLock<CSequentialOutStreamImp>;
      CComPtr<ISequentialOutStream> outStream(outStreamSpec);
      outStreamSpec->Init();
      writeCoderProperties->WriteCoderProperties(outStream);
      
      UINT32 size = outStreamSpec->GetSize();
      
      // encodingInfo.Properties.SetCapacity(size);
      if (encodingInfo.AltCoders.Size() == 0)
        encodingInfo.AltCoders.Add(CAltCoderInfo());
      CAltCoderInfo &altCoderInfo = encodingInfo.AltCoders.Front();
      altCoderInfo.Properties.SetCapacity(size);
      
      memmove(altCoderInfo.Properties, outStreamSpec->GetBuffer(), size);
    }
    
    CComPtr<ICryptoSetPassword> cryptoSetPassword;
    if (methodFull.MethodInfoEx.IsSimpleCoder())
    {
      encoder.QueryInterface(&cryptoSetPassword);
    }
    else
    {
      encoder2.QueryInterface(&cryptoSetPassword);
    }

    if (cryptoSetPassword)
    {
      RETURN_IF_NOT_S_OK(cryptoSetPassword->CryptoSetPassword(
        (const BYTE *)(const wchar_t *)_options.Password, 
        _options.Password.Length() * sizeof(wchar_t)));
    }

    // public ICompressWriteCoderProperties,
    if (methodFull.MethodInfoEx.IsSimpleCoder())
    {
      _encoders.Add(encoder);
      _mixerCoderSpec->AddCoder(encoder);
    }
    else
    {
      _encoders2.Add(encoder2);
      _mixerCoderSpec->AddCoder2(encoder2);
    }
  }
  return S_OK;
}

HRESULT CEncoder::Encode(ISequentialInStream *inStream,
    const UINT64 *inStreamSize,
    CFolderItemInfo &folderItem,
    ISequentialOutStream *outStream,
    CRecordVector<UINT64> &packSizes,
    ICompressProgressInfo *compressProgress)
{
  if (_mixerCoderSpec == NULL)
  {
    RETURN_IF_NOT_S_OK(CreateMixerCoder());
  }
  _mixerCoderSpec->ReInit();
  // _mixerCoderSpec->SetCoderInfo(0, NULL, NULL, progress);

  CObjectVector<CInOutTempBuffer> inOutTempBuffers;
  CObjectVector<CComObjectNoLock<CSequentialOutTempBufferImp> *> tempBufferSpecs;
  CObjectVector<CComPtr<ISequentialOutStream> > tempBuffers;
  int numMethods = _bindInfo.CodersInfo.Size();
  for (int i = 1; i < _bindInfo.OutStreams.Size(); i++)
  {
    inOutTempBuffers.Add(CInOutTempBuffer());
    inOutTempBuffers.Back().Create();
    inOutTempBuffers.Back().InitWriting();
  }
  for (i = 1; i < _bindInfo.OutStreams.Size(); i++)
  {
    CComObjectNoLock<CSequentialOutTempBufferImp> *tempBufferSpec = 
      new CComObjectNoLock<CSequentialOutTempBufferImp>;
    CComPtr<ISequentialOutStream> tempBuffer = tempBufferSpec;
    tempBufferSpec->Init(&inOutTempBuffers[i - 1]);
    tempBuffers.Add(tempBuffer);
    tempBufferSpecs.Add(tempBufferSpec);
  }

  for (i = 0; i < numMethods; i++)
    _mixerCoderSpec->SetCoderInfo(i, NULL, NULL);

  if (_bindInfo.InStreams.IsEmpty())
    return E_FAIL;
  UINT32 mainCoderIndex, mainStreamIndex;
  _bindInfo.FindInStream(_bindInfo.InStreams[0], mainCoderIndex, mainStreamIndex);
  _mixerCoderSpec->SetProgressCoderIndex(mainCoderIndex);
  if (inStreamSize != NULL)
  {
    CRecordVector<const UINT64 *> sizePointers;
    for (int i = 0; i < _bindInfo.CodersInfo[mainCoderIndex].NumInStreams; i++)
      if (i == mainStreamIndex)
        sizePointers.Add(inStreamSize);
      else
        sizePointers.Add(NULL);
    _mixerCoderSpec->SetCoderInfo(mainCoderIndex, &sizePointers.Front(), NULL);
  }

  
  // UINT64 outStreamStartPos;
  // RETURN_IF_NOT_S_OK(stream->Seek(0, STREAM_SEEK_CUR, &outStreamStartPos));
  
  CComObjectNoLock<CSequentialInStreamSizeCount2> *inStreamSizeCountSpec = 
      new CComObjectNoLock<CSequentialInStreamSizeCount2>;
  CComPtr<ISequentialInStream> inStreamSizeCount = inStreamSizeCountSpec;
  CComObjectNoLock<CSequentialOutStreamSizeCount> *outStreamSizeCountSpec = 
      new CComObjectNoLock<CSequentialOutStreamSizeCount>;
  CComPtr<ISequentialOutStream> outStreamSizeCount = outStreamSizeCountSpec;

  inStreamSizeCountSpec->Init(inStream);
  outStreamSizeCountSpec->Init(outStream);

  CRecordVector<ISequentialInStream *> inStreamPointers;
  CRecordVector<ISequentialOutStream *> outStreamPointers;
  inStreamPointers.Add(inStreamSizeCount);
  outStreamPointers.Add(outStreamSizeCount);
  for (i = 1; i < _bindInfo.OutStreams.Size(); i++)
    outStreamPointers.Add(tempBuffers[i - 1]);
  
  RETURN_IF_NOT_S_OK(_mixerCoder->Code(&inStreamPointers.Front(), NULL, 1,
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
  
  for (i = 0; i < _bindReverseConverter->NumSrcInStreams; i++)
  {
    int binder = _bindInfo.FindBinderForInStream(
        _bindReverseConverter->DestOutToSrcInMap[i]);
    UINT64 streamSize;
    if (binder < 0)
      streamSize = inStreamSizeCountSpec->GetSize();
    else
      streamSize = _mixerCoderSpec->GetWriteProcessedSize(binder);
    folderItem.UnPackSizes.Add(streamSize);
  }
  for (i = numMethods - 1; i >= 0; i--)
  {
    // folderItem.CodersInfo[numMethods - 1 - i].Properties = _codersInfo[i].Properties;
    for (int j = 0; j < _codersInfo[i].AltCoders.Size(); j++)
      folderItem.CodersInfo[numMethods - 1 - i].AltCoders[j].Properties 
          = _codersInfo[i].AltCoders[j].Properties;
  }
  return S_OK;
}


CEncoder::CEncoder(const CCompressionMethodMode *options):
  _bindReverseConverter(0)
{
  if (options == 0 || options->IsEmpty())
    throw 1;

  _options = *options;
  _mixerCoderSpec = NULL;

  if (options->Methods.IsEmpty())
  {
    // it has only password method;
    if (!options->PasswordIsDefined)
      throw 1;
    if (!options->Binds.IsEmpty())
      throw 1;
    NCoderMixer2::CCoderStreamsInfo coderStreamsInfo;
    CMethodFull method;
    method.MatchFinderIsDefined = false;
    method.MethodInfoEx.NumInStreams = 1;
    method.MethodInfoEx.NumOutStreams = 1;
    coderStreamsInfo.NumInStreams = method.MethodInfoEx.NumOutStreams;
    coderStreamsInfo.NumOutStreams = method.MethodInfoEx.NumInStreams;
    method.MethodInfoEx.MethodID = k_AES;
    
    #ifndef EXCLUDE_COM
    method.EncoderClassID = CLSID_CCrypto7zAESEncoder;
    #endif
    
    _options.Methods.Add(method);
    _bindInfo.CodersInfo.Add(coderStreamsInfo);
  
    _bindInfo.InStreams.Add(0);
    _bindInfo.OutStreams.Add(0);
  }
  else
  {

  UINT32 numInStreams = 0, numOutStreams = 0;
  int i;
  for (i = 0; i < options->Methods.Size(); i++)
  {
    const CMethodFull &methodFull = options->Methods[i];
    NCoderMixer2::CCoderStreamsInfo coderStreamsInfo;
    coderStreamsInfo.NumInStreams = methodFull.MethodInfoEx.NumOutStreams;
    coderStreamsInfo.NumOutStreams = methodFull.MethodInfoEx.NumInStreams;
    if (options->Binds.IsEmpty())
    {
      if (i < options->Methods.Size() - 1)
      {
        NCoderMixer2::CBindPair bindPair;
        bindPair.InIndex = numInStreams + coderStreamsInfo.NumInStreams;
        bindPair.OutIndex = numOutStreams;
        _bindInfo.BindPairs.Add(bindPair);
      }
      else
        _bindInfo.OutStreams.Insert(0, numOutStreams);
      for (UINT32 j = 1; j < coderStreamsInfo.NumOutStreams; j++)
        _bindInfo.OutStreams.Add(numOutStreams + j);
    }
    
    numInStreams += coderStreamsInfo.NumInStreams;
    numOutStreams += coderStreamsInfo.NumOutStreams;

    _bindInfo.CodersInfo.Add(coderStreamsInfo);
  }

  if (!options->Binds.IsEmpty())
  {
    for (int i = 0; i < options->Binds.Size(); i++)
    {
      NCoderMixer2::CBindPair bindPair;
      const CBind &bind = options->Binds[i];
      bindPair.InIndex = _bindInfo.GetCoderInStreamIndex(bind.InCoder) + bind.InStream;
      bindPair.OutIndex = _bindInfo.GetCoderOutStreamIndex(bind.OutCoder) + bind.OutStream;
      _bindInfo.BindPairs.Add(bindPair);
    }
    for (i = 0; i < numOutStreams; i++)
      if (_bindInfo.FindBinderForOutStream(i) == -1)
        _bindInfo.OutStreams.Add(i);
  }

  for (i = 0; i < numInStreams; i++)
    if (_bindInfo.FindBinderForInStream(i) == -1)
      _bindInfo.InStreams.Add(i);

  if (_bindInfo.InStreams.IsEmpty())
    throw 1; // this is error

  // Make main stream first in list
  int inIndex = _bindInfo.InStreams[0];
  while (true)
  {
    UINT32 coderIndex, coderStreamIndex;
    _bindInfo.FindInStream(inIndex, coderIndex, coderStreamIndex);
    UINT32 outIndex = _bindInfo.GetCoderStartOutStream(coderIndex);
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
      method.MatchFinderIsDefined = false;
      method.MethodInfoEx.NumInStreams = 1;
      method.MethodInfoEx.NumOutStreams = 1;
      coderStreamsInfo.NumInStreams = method.MethodInfoEx.NumOutStreams;
      coderStreamsInfo.NumOutStreams = method.MethodInfoEx.NumInStreams;
      method.MethodInfoEx.MethodID = k_AES;

      #ifndef EXCLUDE_COM
      method.EncoderClassID = CLSID_CCrypto7zAESEncoder;
      #endif

      _options.Methods.Add(method);
      _bindInfo.CodersInfo.Add(coderStreamsInfo);
      _bindInfo.OutStreams.Add(numOutStreams + i);
    }
  }

  }

  for (int i = _options.Methods.Size() - 1; i >= 0; i--)
  {
    const CMethodFull &methodFull = _options.Methods[i];
    _decompressionMethods.Add(methodFull.MethodInfoEx.MethodID);
  }

  _bindReverseConverter = new NCoderMixer2::CBindReverseConverter(_bindInfo);
  _bindReverseConverter->CreateReverseBindInfo(_decompressBindInfo);
}

CEncoder::~CEncoder()
{
  delete _bindReverseConverter;
}

}}
