// Decode.cpp

#include "StdAfx.h"

#include "Decode.h"

#include "Util/MultiStream.h"

#include "Interface/StreamObjects.h"
#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"

#include "MethodInfo.h"

using namespace NArchive;
using namespace N7z;

#ifdef COMPRESS_LZMA
#include "../../../Compress/LZ/LZMA/Decoder.h"
static CMethodID k_LZMA = { { 0x3, 0x1, 0x1 }, 3 };
#endif

#ifdef COMPRESS_PPMD
#include "../../../Compress/PPM/PPMD/Decoder.h"
static CMethodID k_PPMD = { { 0x3, 0x4, 0x1 }, 3 };
#endif

#ifdef COMPRESS_BCJ_X86
#include "../../../Compress/Convert/Branch/x86.h"
static CMethodID k_BCJ_X86 = { { 0x3, 0x3, 0x1, 0x3 }, 4 };
#endif

#ifdef COMPRESS_DEFLATE
#include "../../../Compress/LZ/Deflate/Decoder.h"
static CMethodID k_Deflate = { { 0x4, 0x1, 0x8 }, 3 };
#endif

#ifdef COMPRESS_BZIP2
#include "../../../Compress/BWT/BZip2/Decoder.h"
static CMethodID k_BZip2 = { { 0x4, 0x2, 0x2 }, 3 };
#endif

#ifdef COMPRESS_COPY
#include "Compression/CopyCoder.h"
static CMethodID k_Copy = { { 0x0 }, 1 };
#endif


static void ConvertFolderItemInfoToBindInfo(const CFolderItemInfo &aFolderItemInfo,
    CBindInfoEx &aBindInfo)
{
  aBindInfo.CodersInfo.Clear();
  aBindInfo.CoderMethodIDs.Clear();
  aBindInfo.OutStreams.Clear();
  aBindInfo.InStreams.Clear();
  aBindInfo.BindPairs.Clear();
  for (int i = 0; i < aFolderItemInfo.BindPairs.Size(); i++)
  {
    NCoderMixer2::CBindPair aBindPair;
    aBindPair.InIndex = aFolderItemInfo.BindPairs[i].InIndex;
    aBindPair.OutIndex = aFolderItemInfo.BindPairs[i].OutIndex;
    aBindInfo.BindPairs.Add(aBindPair);
  }
  UINT32 anOutStreamIndex = 0;
  for (i = 0; i < aFolderItemInfo.CodersInfo.Size(); i++)
  {
    NCoderMixer2::CCoderStreamsInfo aCoderStreamsInfo;
    const CCoderInfo &aCoderInfo = aFolderItemInfo.CodersInfo[i];
    aCoderStreamsInfo.NumInStreams = aCoderInfo.NumInStreams;
    aCoderStreamsInfo.NumOutStreams = aCoderInfo.NumOutStreams;
    aBindInfo.CodersInfo.Add(aCoderStreamsInfo);
    aBindInfo.CoderMethodIDs.Add(aCoderInfo.DecompressionMethod);
    for (int j = 0; j < aCoderStreamsInfo.NumOutStreams; j++, anOutStreamIndex++)
      if (aFolderItemInfo.FindBindPairForOutStream(anOutStreamIndex) < 0)
        aBindInfo.OutStreams.Add(anOutStreamIndex);
  }
  for (i = 0; i < aFolderItemInfo.PackStreams.Size(); i++)
    aBindInfo.InStreams.Add(aFolderItemInfo.PackStreams[i].Index);
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
  BindInfoExPrevIsDefinded = false;
}

HRESULT CDecoder::Decode(IInStream *anInStream,
    UINT64 aStartPos,
    const UINT64 *aPackSizes,
    const CFolderItemInfo &aFolderInfo, 
    ISequentialOutStream *anOutStream,
    ICompressProgressInfo *aCompressProgress)
{
  CObjectVector< CComPtr<ISequentialInStream> > anInStreams;
  
  CLockedInStream aLockedInStream;
  aLockedInStream.Init(anInStream);
  
  for (int j = 0; j < aFolderInfo.PackStreams.Size(); j++)
  {
    CComObjectNoLock<CLockedSequentialInStreamImp> *aLockedStreamImpSpec = new 
      CComObjectNoLock<CLockedSequentialInStreamImp>;
    CComPtr<ISequentialInStream> aLockedStreamImp = aLockedStreamImpSpec;
    aLockedStreamImpSpec->Init(&aLockedInStream, aStartPos);
    aStartPos += aPackSizes[j];
    
    CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
      CComObjectNoLock<CLimitedSequentialInStream>;
    CComPtr<ISequentialInStream> anInStream = aStreamSpec;
    aStreamSpec->Init(aLockedStreamImp, aPackSizes[j]);
    anInStreams.Add(anInStream);
  }
  
  int aNumCoders = aFolderInfo.CodersInfo.Size();
  
  CBindInfoEx aBindInfo;
  ConvertFolderItemInfoToBindInfo(aFolderInfo, aBindInfo);
  bool aCreateNewCoders;
  if (!BindInfoExPrevIsDefinded)
    aCreateNewCoders = true;
  else
    aCreateNewCoders = !AreBindInfoExEqual(aBindInfo, BindInfoExPrev);
  if (aCreateNewCoders)
  {
    int i;
    aDecoders.Clear();
    aDecoders2.Clear();
    
    MixerCoder.Release();
    
    MixerCoderSpec = new CComObjectNoLock<NCoderMixer2::CCoderMixer2>;
    MixerCoder = MixerCoderSpec;
    
    MixerCoderSpec->SetBindInfo(aBindInfo);
    for (i = 0; i < aNumCoders; i++)
    {
      const CCoderInfo &aCoderInfo = aFolderInfo.CodersInfo[i];

      #ifndef EXCLUDE_COM
      CLSID aClassID;
      if (!aMethodMap.GetCLSIDAlways(aCoderInfo.DecompressionMethod, aClassID)) 
        return E_FAIL;
      #endif

      if (aCoderInfo.IsSimpleCoder())
      {
        aDecoders.Add(CComPtr<ICompressCoder>());

        #ifdef COMPRESS_LZMA
        if (aCoderInfo.DecompressionMethod == k_LZMA)
          aDecoders.Back() = new CComObjectNoLock<NCompress::NLZMA::CDecoder>;
        #endif

        #ifdef COMPRESS_PPMD
        if (aCoderInfo.DecompressionMethod == k_PPMD)
          aDecoders.Back() = new CComObjectNoLock<NCompress::NPPMD::CDecoder>;
        #endif

        #ifdef COMPRESS_BCJ_X86
        if (aCoderInfo.DecompressionMethod == k_BCJ_X86)
          aDecoders.Back() = new CComObjectNoLock<CBCJ_x86_Decoder>;
        #endif

        #ifdef COMPRESS_DEFLATE
        if (aCoderInfo.DecompressionMethod == k_Deflate)
          aDecoders.Back() = new CComObjectNoLock<NDeflate::NDecoder::CCoder>;
        #endif

        #ifdef COMPRESS_BZIP2
        if (aCoderInfo.DecompressionMethod == k_BZip2)
          aDecoders.Back() = new CComObjectNoLock<NCompress::NBZip2::NDecoder::CCoder>;
        #endif

        #ifdef COMPRESS_COPY
        if (aCoderInfo.DecompressionMethod == k_Copy)
          aDecoders.Back() = new CComObjectNoLock<NCompression::CCopyCoder>;
        #endif

        #ifndef EXCLUDE_COM
        if (aDecoders.Back() == 0)
        {
          RETURN_IF_NOT_S_OK(aDecoders.Back().CoCreateInstance(aClassID));
        }
        #endif

        if (aDecoders.Back() == 0)
          return E_FAIL;

        MixerCoderSpec->AddCoder(aDecoders.Back());
      }
      else
      {
        #ifndef EXCLUDE_COM
        aDecoders2.Add(CComPtr<ICompressCoder2>());
        RETURN_IF_NOT_S_OK(aDecoders2.Back().CoCreateInstance(aClassID));
        MixerCoderSpec->AddCoder2(aDecoders2.Back());
        #else
        return E_FAIL;
        #endif
      }
    }
    BindInfoExPrev = aBindInfo;
    BindInfoExPrevIsDefinded = true;
  }
  int i;
  MixerCoderSpec->ReInit();
  
  UINT32 aPackStreamIndex = 0, anUnPackStreamIndex = 0;
  UINT32 aCoderIndex = 0;
  UINT32 aCoder2Index = 0;
  
  for (i = 0; i < aNumCoders; i++)
  {
    CComPtr<ICompressSetDecoderProperties> aCompressSetDecoderProperties;
    HRESULT aResult;
    if (aFolderInfo.CodersInfo[i].IsSimpleCoder())
      aResult = aDecoders[aCoderIndex++]->QueryInterface(&aCompressSetDecoderProperties);
    else
      aResult = aDecoders2[aCoder2Index++]->QueryInterface(&aCompressSetDecoderProperties);
    
    if (aResult == S_OK)
    {
      const CByteBuffer &aProperties = aFolderInfo.CodersInfo[i].Properties;
      UINT32 aSize = aProperties.GetCapacity();
      if (aSize > 0)
      {
        CComObjectNoLock<CSequentialInStreamImp> *anInStreamSpec = new 
          CComObjectNoLock<CSequentialInStreamImp>;
        CComPtr<ISequentialInStream> anInStream(anInStreamSpec);
        anInStreamSpec->Init((const BYTE *)aProperties, aSize);
        RETURN_IF_NOT_S_OK(aCompressSetDecoderProperties->SetDecoderProperties(anInStream));
      }
    }
    else if (aResult != E_NOINTERFACE)
      return aResult;
    
    const CCoderInfo &aCoderInfo = aFolderInfo.CodersInfo[i];
    UINT32 aNumInStreams = aCoderInfo.NumInStreams;
    UINT32 aNumOutStreams = aCoderInfo.NumOutStreams;
    CRecordVector<const UINT64 *> aPackSizesPointers;
    CRecordVector<const UINT64 *> anUnPackSizesPointers;
    aPackSizesPointers.Reserve(aNumInStreams);
    anUnPackSizesPointers.Reserve(aNumOutStreams);
    for (int j = 0; j < aNumOutStreams; j++, anUnPackStreamIndex++)
      anUnPackSizesPointers.Add(&aFolderInfo.UnPackSizes[anUnPackStreamIndex]);
    
    for (j = 0; j < aNumInStreams; j++, aPackStreamIndex++)
    {
      int aBindPairIndex = aFolderInfo.FindBindPairForInStream(aPackStreamIndex);
      if (aBindPairIndex >= 0)
        aPackSizesPointers.Add(
        &aFolderInfo.UnPackSizes[aFolderInfo.BindPairs[aBindPairIndex].OutIndex]);
      else
      {
        int anIndex = aFolderInfo.FindPackStreamArrayIndex(aPackStreamIndex);
        if (anIndex < 0)
          return E_FAIL;
        aPackSizesPointers.Add(&aPackSizes[anIndex]);
      }
    }
    
    MixerCoderSpec->SetCoderInfo(i, 
      &aPackSizesPointers.Front(), 
      &anUnPackSizesPointers.Front());
    MixerCoderSpec->SetProgressCoderIndex(0);
  }
  
  if (aNumCoders == 0)
    return 0;
  CRecordVector<ISequentialInStream *> anInStreamPointers;
  anInStreamPointers.Reserve(anInStreams.Size());
  for (i = 0; i < anInStreams.Size(); i++)
    anInStreamPointers.Add(anInStreams[i]);
  ISequentialOutStream *anOutStreamPointer = anOutStream;
  return MixerCoderSpec->Code(&anInStreamPointers.Front(), NULL, 
    anInStreams.Size(), &anOutStreamPointer, NULL, 1, aCompressProgress);
}
