// Decode.cpp

#include "StdAfx.h"

#include "7zDecode.h"

#include "../../Common/MultiStream.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"

#include "7zMethods.h"

#include "../../IPassword.h"

using namespace NArchive;
using namespace N7z;

#ifdef COMPRESS_LZMA
#include "../../Compress/LZMA/LZMADecoder.h"
static CMethodID k_LZMA = { { 0x3, 0x1, 0x1 }, 3 };
#endif

#ifdef COMPRESS_PPMD
#include "../../Compress/PPMD/PPMDDecoder.h"
static CMethodID k_PPMD = { { 0x3, 0x4, 0x1 }, 3 };
#endif

#ifdef COMPRESS_BCJ_X86
#include "../../Compress/Branch/x86.h"
static CMethodID k_BCJ_X86 = { { 0x3, 0x3, 0x1, 0x3 }, 4 };
#endif

#ifdef COMPRESS_BCJ2
#include "../../Compress/Branch/x86_2.h"
static CMethodID k_BCJ2 = { { 0x3, 0x3, 0x1, 0x1B }, 4 };
#endif

#ifdef COMPRESS_DEFLATE
#include "../../Compress/Deflate/DeflateDecoder.h"
static CMethodID k_Deflate = { { 0x4, 0x1, 0x8 }, 3 };
#endif

#ifdef COMPRESS_BZIP2
#include "../../Compress/BZip2/BZip2Decoder.h"
static CMethodID k_BZip2 = { { 0x4, 0x2, 0x2 }, 3 };
#endif

#ifdef COMPRESS_COPY
#include "../../Compress/Copy/CopyCoder.h"
static CMethodID k_Copy = { { 0x0 }, 1 };
#endif

#ifdef CRYPTO_7ZAES
#include "../../Crypto/7zAES/7zAES.h"
static CMethodID k_7zAES = { { 0x6, 0xF1, 0x07, 0x01 }, 4 };
#endif


static void ConvertFolderItemInfoToBindInfo(const CFolderItemInfo &folderItemInfo,
    CBindInfoEx &bindInfo)
{
  bindInfo.CodersInfo.Clear();
  bindInfo.CoderMethodIDs.Clear();
  bindInfo.OutStreams.Clear();
  bindInfo.InStreams.Clear();
  bindInfo.BindPairs.Clear();
  for (int i = 0; i < folderItemInfo.BindPairs.Size(); i++)
  {
    NCoderMixer2::CBindPair bindPair;
    bindPair.InIndex = (UINT32)folderItemInfo.BindPairs[i].InIndex;
    bindPair.OutIndex = (UINT32)folderItemInfo.BindPairs[i].OutIndex;
    bindInfo.BindPairs.Add(bindPair);
  }
  UINT32 outStreamIndex = 0;
  for (i = 0; i < folderItemInfo.CodersInfo.Size(); i++)
  {
    NCoderMixer2::CCoderStreamsInfo coderStreamsInfo;
    const CCoderInfo &coderInfo = folderItemInfo.CodersInfo[i];
    coderStreamsInfo.NumInStreams = (UINT32)coderInfo.NumInStreams;
    coderStreamsInfo.NumOutStreams = (UINT32)coderInfo.NumOutStreams;
    bindInfo.CodersInfo.Add(coderStreamsInfo);
    const CAltCoderInfo &altCoderInfo = coderInfo.AltCoders.Front();
    bindInfo.CoderMethodIDs.Add(altCoderInfo.DecompressionMethod);
    for (UINT32 j = 0; j < coderStreamsInfo.NumOutStreams; j++, outStreamIndex++)
      if (folderItemInfo.FindBindPairForOutStream(outStreamIndex) < 0)
        bindInfo.OutStreams.Add(outStreamIndex);
  }
  for (i = 0; i < folderItemInfo.PackStreams.Size(); i++)
    bindInfo.InStreams.Add((UINT32)folderItemInfo.PackStreams[i].Index);
}

static bool AreCodersEqual(const NCoderMixer2::CCoderStreamsInfo &a1, 
    const NCoderMixer2::CCoderStreamsInfo &a2)
{
  return (a1.NumInStreams == a2.NumInStreams) &&
    (a1.NumOutStreams == a2.NumOutStreams);
}

static bool AreBindPairsEqual(const NCoderMixer2::CBindPair &a1, const NCoderMixer2::CBindPair &a2)
{
  return (a1.InIndex == a2.InIndex) &&
    (a1.OutIndex == a2.OutIndex);
}

static bool AreBindInfoExEqual(const CBindInfoEx &a1, const CBindInfoEx &a2)
{
  if (a1.CodersInfo.Size() != a2.CodersInfo.Size())
    return false;
  for (int i = 0; i < a1.CodersInfo.Size(); i++)
    if (!AreCodersEqual(a1.CodersInfo[i], a2.CodersInfo[i]))
      return false;
  if (a1.BindPairs.Size() != a2.BindPairs.Size())
    return false;
  for (i = 0; i < a1.BindPairs.Size(); i++)
    if (!AreBindPairsEqual(a1.BindPairs[i], a2.BindPairs[i]))
      return false;
  for (i = 0; i < a1.CoderMethodIDs.Size(); i++)
    if (a1.CoderMethodIDs[i] != a2.CoderMethodIDs[i])
      return false;
  if (a1.InStreams.Size() != a2.InStreams.Size())
    return false;
  if (a1.OutStreams.Size() != a2.OutStreams.Size())
    return false;
  return true;
}

CDecoder::CDecoder()
{
  _bindInfoExPrevIsDefinded = false;
  #ifndef EXCLUDE_COM
  LoadMethodMap();
  #endif
}

HRESULT CDecoder::Decode(IInStream *inStream,
    UINT64 startPos,
    const UINT64 *packSizes,
    const CFolderItemInfo &folderInfo, 
    ISequentialOutStream *outStream,
    ICompressProgressInfo *compressProgress
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword
    #endif
    )
{
  CObjectVector< CMyComPtr<ISequentialInStream> > inStreams;
  
  CLockedInStream lockedInStream;
  lockedInStream.Init(inStream);
  
  for (int j = 0; j < folderInfo.PackStreams.Size(); j++)
  {
    CLockedSequentialInStreamImp *lockedStreamImpSpec = new 
        CLockedSequentialInStreamImp;
    CMyComPtr<ISequentialInStream> lockedStreamImp = lockedStreamImpSpec;
    lockedStreamImpSpec->Init(&lockedInStream, startPos);
    startPos += packSizes[j];
    
    CLimitedSequentialInStream *streamSpec = new 
        CLimitedSequentialInStream;
    CMyComPtr<ISequentialInStream> inStream = streamSpec;
    streamSpec->Init(lockedStreamImp, packSizes[j]);
    inStreams.Add(inStream);
  }
  
  int numCoders = folderInfo.CodersInfo.Size();
  
  CBindInfoEx bindInfo;
  ConvertFolderItemInfoToBindInfo(folderInfo, bindInfo);
  bool createNewCoders;
  if (!_bindInfoExPrevIsDefinded)
    createNewCoders = true;
  else
    createNewCoders = !AreBindInfoExEqual(bindInfo, _bindInfoExPrev);
  if (createNewCoders)
  {
    int i;
    _decoders.Clear();
    // _decoders2.Clear();
    
    _mixerCoder.Release();
    
    _mixerCoderSpec = new NCoderMixer2::CCoderMixer2;
    _mixerCoder = _mixerCoderSpec;
    
    _mixerCoderSpec->SetBindInfo(bindInfo);
    for (i = 0; i < numCoders; i++)
    {
      const CCoderInfo &coderInfo = folderInfo.CodersInfo[i];
      const CAltCoderInfo &altCoderInfo = coderInfo.AltCoders.Front();
      #ifndef EXCLUDE_COM
      CMethodInfo methodInfo;
      if (!GetMethodInfo(altCoderInfo.DecompressionMethod, methodInfo)) 
        return E_NOTIMPL;
      #endif

      if (coderInfo.IsSimpleCoder())
      {
        CMyComPtr<ICompressCoder> decoder;

        #ifdef COMPRESS_LZMA
        if (altCoderInfo.DecompressionMethod == k_LZMA)
          decoder = new NCompress::NLZMA::CDecoder;
        #endif

        #ifdef COMPRESS_PPMD
        if (altCoderInfo.DecompressionMethod == k_PPMD)
          decoder = new NCompress::NPPMD::CDecoder;
        #endif

        #ifdef COMPRESS_BCJ_X86
        if (altCoderInfo.DecompressionMethod == k_BCJ_X86)
          decoder = new CBCJ_x86_Decoder;
        #endif

        #ifdef COMPRESS_DEFLATE
        if (altCoderInfo.DecompressionMethod == k_Deflate)
          decoder = new NCompress::NDeflate::NDecoder::CCOMCoder;
        #endif

        #ifdef COMPRESS_BZIP2
        if (altCoderInfo.DecompressionMethod == k_BZip2)
          decoder = new NCompress::NBZip2::CDecoder;
        #endif

        #ifdef COMPRESS_COPY
        if (altCoderInfo.DecompressionMethod == k_Copy)
          decoder = new NCompress::CCopyCoder;
        #endif

        #ifdef CRYPTO_7ZAES
        if (altCoderInfo.DecompressionMethod == k_7zAES)
          decoder = new NCrypto::NSevenZ::CDecoder;
        #endif

        #ifndef EXCLUDE_COM
        if (decoder == 0)
        {
          RINOK(_libraries.CreateCoder(methodInfo.FilePath, 
              methodInfo.Decoder, &decoder));
        }
        #endif

        if (decoder == 0)
          return E_NOTIMPL;

        _decoders.Add((IUnknown *)decoder);

        _mixerCoderSpec->AddCoder(decoder);
      }
      else
      {
        CMyComPtr<ICompressCoder2> decoder;

        #ifdef COMPRESS_BCJ2
        if (altCoderInfo.DecompressionMethod == k_BCJ2)
          decoder = new CBCJ2_x86_Decoder;
        #endif

        #ifndef EXCLUDE_COM
        if (decoder == 0)
        {
          RINOK(_libraries.CreateCoder2(methodInfo.FilePath, 
              methodInfo.Decoder, &decoder));
        }
        #endif

        if (decoder == 0)
          return E_NOTIMPL;

        _decoders.Add((IUnknown *)decoder);
        _mixerCoderSpec->AddCoder2(decoder);
      }
    }
    _bindInfoExPrev = bindInfo;
    _bindInfoExPrevIsDefinded = true;
  }
  int i;
  _mixerCoderSpec->ReInit();
  
  UINT32 packStreamIndex = 0, unPackStreamIndex = 0;
  UINT32 coderIndex = 0;
  // UINT32 coder2Index = 0;
  
  for (i = 0; i < numCoders; i++)
  {
    const CCoderInfo &coderInfo = folderInfo.CodersInfo[i];
    const CAltCoderInfo &altCoderInfo = coderInfo.AltCoders.Front();
    CMyComPtr<ICompressSetDecoderProperties> compressSetDecoderProperties;
    HRESULT result = _decoders[coderIndex].QueryInterface(
        IID_ICompressSetDecoderProperties, &compressSetDecoderProperties);
    
    if (result == S_OK)
    {
      const CByteBuffer &properties = altCoderInfo.Properties;
      UINT32 size = properties.GetCapacity();
      if (size > 0)
      {
        CSequentialInStreamImp *inStreamSpec = new CSequentialInStreamImp;
        CMyComPtr<ISequentialInStream> inStream(inStreamSpec);
        inStreamSpec->Init((const BYTE *)properties, size);
        RINOK(compressSetDecoderProperties->SetDecoderProperties(inStream));
      }
    }
    else if (result != E_NOINTERFACE)
      return result;

    #ifndef _NO_CRYPTO
    CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
    result = _decoders[coderIndex].QueryInterface(
        IID_ICryptoSetPassword, &cryptoSetPassword);

    if (result == S_OK)
    {
      if (getTextPassword == 0)
        return E_FAIL;
      CMyComBSTR password;
      RINOK(getTextPassword->CryptoGetTextPassword(&password));
      UString unicodePassword = password;
      RINOK(cryptoSetPassword->CryptoSetPassword(
        (const BYTE *)(const wchar_t *)unicodePassword, 
        unicodePassword.Length() * sizeof(wchar_t)));
    }
    else if (result != E_NOINTERFACE)
      return result;
    #endif

    coderIndex++;
    
    UINT32 numInStreams = (UINT32)coderInfo.NumInStreams;
    UINT32 numOutStreams = (UINT32)coderInfo.NumOutStreams;
    CRecordVector<const UINT64 *> packSizesPointers;
    CRecordVector<const UINT64 *> unPackSizesPointers;
    packSizesPointers.Reserve(numInStreams);
    unPackSizesPointers.Reserve(numOutStreams);
    for (UINT32 j = 0; j < numOutStreams; j++, unPackStreamIndex++)
      unPackSizesPointers.Add(&folderInfo.UnPackSizes[unPackStreamIndex]);
    
    for (j = 0; j < numInStreams; j++, packStreamIndex++)
    {
      int bindPairIndex = folderInfo.FindBindPairForInStream(packStreamIndex);
      if (bindPairIndex >= 0)
        packSizesPointers.Add(
        &folderInfo.UnPackSizes[(UINT32)folderInfo.BindPairs[bindPairIndex].OutIndex]);
      else
      {
        int index = folderInfo.FindPackStreamArrayIndex(packStreamIndex);
        if (index < 0)
          return E_FAIL;
        packSizesPointers.Add(&packSizes[index]);
      }
    }
    
    _mixerCoderSpec->SetCoderInfo(i, 
      &packSizesPointers.Front(), 
      &unPackSizesPointers.Front());
  }
  UINT32 mainCoder, temp;
  bindInfo.FindOutStream(bindInfo.OutStreams[0], mainCoder, temp);
  _mixerCoderSpec->SetProgressCoderIndex(mainCoder);
  
  if (numCoders == 0)
    return 0;
  CRecordVector<ISequentialInStream *> inStreamPointers;
  inStreamPointers.Reserve(inStreams.Size());
  for (i = 0; i < inStreams.Size(); i++)
    inStreamPointers.Add(inStreams[i]);
  ISequentialOutStream *outStreamPointer = outStream;
  return _mixerCoderSpec->Code(&inStreamPointers.Front(), NULL, 
    inStreams.Size(), &outStreamPointer, NULL, 1, compressProgress);
}
