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

void ConvertBindInfoToFolderItemInfo(const NCoderMixer2::CBindInfo &aBindInfo,
    const CRecordVector<CMethodID> m_DecompressionMethods,
    CFolderItemInfo &aFolderItemInfo)
{
  aFolderItemInfo.CodersInfo.Clear();
  // aBindInfo.CoderMethodIDs.Clear();
  // aFolderItemInfo.OutStreams.Clear();
  aFolderItemInfo.PackStreams.Clear();
  aFolderItemInfo.BindPairs.Clear();
  for (int i = 0; i < aBindInfo.BindPairs.Size(); i++)
  {
    CBindPair aBindPair;
    aBindPair.InIndex = aBindInfo.BindPairs[i].InIndex;
    aBindPair.OutIndex = aBindInfo.BindPairs[i].OutIndex;
    aFolderItemInfo.BindPairs.Add(aBindPair);
  }
  for (i = 0; i < aBindInfo.CodersInfo.Size(); i++)
  {
    CCoderInfo aCoderInfo;
    const NCoderMixer2::CCoderStreamsInfo &aCoderStreamsInfo = aBindInfo.CodersInfo[i];
    aCoderInfo.NumInStreams = aCoderStreamsInfo.NumInStreams;
    aCoderInfo.NumOutStreams = aCoderStreamsInfo.NumOutStreams;
    aCoderInfo.DecompressionMethod = m_DecompressionMethods[i];
    aFolderItemInfo.CodersInfo.Add(aCoderInfo);
  }
  for (i = 0; i < aBindInfo.InStreams.Size(); i++)
  {
    CPackStreamInfo aPackStreamInfo;
    aPackStreamInfo.Index = aBindInfo.InStreams[i];
    aFolderItemInfo.PackStreams.Add(aPackStreamInfo);
  }
}

HRESULT CEncoder::Encode(ISequentialInStream *anInStream,
    const UINT64 *anInStreamSize,
    CFolderItemInfo &aFolderItem,
    ISequentialOutStream *anOutStream,
    CRecordVector<UINT64> &aPackSizes,
    ICompressProgressInfo *aCompressProgress)
{
  if (aMixerCoderSpec == NULL)
  {
    aMixerCoderSpec = new CComObjectNoLock<NCoderMixer2::CCoderMixer2>;
    aMixerCoder = aMixerCoderSpec;
    aMixerCoderSpec->SetBindInfo(m_BindInfo);
    for (int i = 0; i < m_Options.Methods.Size(); i++)
    {
      const CMethodFull &aMethodFull = m_Options.Methods[i];
      m_CodersInfo.Add(CCoderInfo());
      CCoderInfo &anEncodingInfo = m_CodersInfo.Back();
      CComPtr<ICompressCoder> anEncoder;
      CComPtr<ICompressCoder2> anEncoder2;
      
      if (aMethodFull.MethodInfoEx.IsSimpleCoder())
      {
        #ifdef COMPRESS_LZMA
        if (aMethodFull.MethodInfoEx.MethodID == k_LZMA)
          anEncoder = new CComObjectNoLock<NCompress::NLZMA::CEncoder>;
        #endif

        #ifdef COMPRESS_PPMD
        if (aMethodFull.MethodInfoEx.MethodID == k_PPMD)
          anEncoder = new CComObjectNoLock<NCompress::NPPMD::CEncoder>;
        #endif

        #ifdef COMPRESS_BCJ_X86
        if (aMethodFull.MethodInfoEx.MethodID == k_BCJ_X86)
          anEncoder = new CComObjectNoLock<CBCJ_x86_Encoder>;
        #endif

        #ifdef COMPRESS_COPY
        if (aMethodFull.MethodInfoEx.MethodID == k_Copy)
          anEncoder = new CComObjectNoLock<NCompression::CCopyCoder>;
        #endif

        #ifdef COMPRESS_BZIP2
        if (aMethodFull.MethodInfoEx.MethodID == k_BZip2)
          anEncoder = new CComObjectNoLock<NCompress::NBZip2::NEncoder::CCoder>;
        #endif

        #ifdef COMPRESS_DEFLATE
        if (aMethodFull.MethodInfoEx.MethodID == k_Deflate)
          anEncoder = new CComObjectNoLock<NDeflate::NEncoder::CCoder>;
        #endif

        #ifndef EXCLUDE_COM
        if (anEncoder == 0)
        {
          RETURN_IF_NOT_S_OK(anEncoder.CoCreateInstance(aMethodFull.EncoderClassID));
        }
        #endif

        if (anEncoder == 0)
          return E_FAIL;

      }
      else
      {
        #ifdef COMPRESS_BCJ2
        if (aMethodFull.MethodInfoEx.MethodID == k_BCJ2)
          anEncoder2 = new CComObjectNoLock<CBCJ2_x86_Encoder>;
        #endif

        #ifndef EXCLUDE_COM
        if (anEncoder2 == 0)
        {
          RETURN_IF_NOT_S_OK(anEncoder2.CoCreateInstance(aMethodFull.EncoderClassID));
        }
        #else
        
        if (anEncoder2 == 0)
          return E_FAIL;
        #endif
      }
      
      
      if (aMethodFull.MatchFinderIsDefined)
      {
        CComPtr<IInWindowStreamMatch> aMatchFinder;
        #ifdef COMPRESS_MF_PAT
        if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("Pat2")) == 0)
          aMatchFinder = new CComObjectNoLock<NPat2::CPatricia>;
        else if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("Pat2H")) == 0)
          aMatchFinder = new CComObjectNoLock<NPat2H::CPatricia>;
        else if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("Pat2R")) == 0)
          aMatchFinder = new CComObjectNoLock<NPat2R::CPatricia>;
        else if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("Pat3H")) == 0)
          aMatchFinder = new CComObjectNoLock<NPat3H::CPatricia>;
        else if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("Pat4H")) == 0)
          aMatchFinder = new CComObjectNoLock<NPat4H::CPatricia>;
        #endif

        #ifdef COMPRESS_MF_BT
        if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("BT2")) == 0)
          aMatchFinder = new CComObjectNoLock<NBT2::CMatchFinderBinTree>;
        else if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("BT3")) == 0)
          aMatchFinder = new CComObjectNoLock<NBT3::CMatchFinderBinTree>;
        else if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("BT4")) == 0)
          aMatchFinder = new CComObjectNoLock<NBT4::CMatchFinderBinTree>;
        else if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("BT4b")) == 0)
          aMatchFinder = new CComObjectNoLock<NBT4b::CMatchFinderBinTree>;
        #endif

        #ifdef COMPRESS_MF_HC
        if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("HC3")) == 0)
          aMatchFinder = new CComObjectNoLock<NHC3::CMatchFinderHC>;
        else if (aMethodFull.MatchFinderName.CompareNoCase(_TEXT("HC4")) == 0)
          aMatchFinder = new CComObjectNoLock<NHC4::CMatchFinderHC>;
        #endif

        #ifndef EXCLUDE_COM
        if (aMatchFinder == 0)
        {
          RETURN_IF_NOT_S_OK(aMatchFinder.CoCreateInstance(aMethodFull.MatchFinderClassID));
        }
        #endif

        if (aMatchFinder == 0)
          return E_FAIL;

        CComPtr<IInitMatchFinder> anInitMatchFinder;
        if (aMethodFull.MethodInfoEx.IsSimpleCoder())
        {
          RETURN_IF_NOT_S_OK(anEncoder->QueryInterface(&anInitMatchFinder));
        }
        else
        {
          RETURN_IF_NOT_S_OK(anEncoder2->QueryInterface(&anInitMatchFinder));
        }
        
        if (m_Options.m_MultiThread)
        {
          CComObjectNoLock<CMatchFinderMT> *aMatchFinderMTSpec = new 
              CComObjectNoLock<CMatchFinderMT>;
          CComPtr<IInWindowStreamMatch> aMatchFinderMT = aMatchFinderMTSpec;
          aMatchFinderMTSpec->m_MultiThreadMult = m_Options.m_MultiThreadMult;
          RETURN_IF_NOT_S_OK(aMatchFinderMTSpec->SetMatchFinder(aMatchFinder));
          anInitMatchFinder->InitMatchFinder(aMatchFinderMT);
        }
        else
          anInitMatchFinder->InitMatchFinder(aMatchFinder);
      }
      
      if (aMethodFull.EncoderProperties.Size() > 0)
      {
        std::vector<NWindows::NCOM::CPropVariant> aProperties;
        std::vector<PROPID> aPropIDs;
        INT32 aNumProperties = aMethodFull.EncoderProperties.Size();
        for (int i = 0; i < aNumProperties; i++)
        {
          const CProperty &aProperty = aMethodFull.EncoderProperties[i];
          aPropIDs.push_back(aProperty.PropID);
          aProperties.push_back(aProperty.Value);
        }
        CComPtr<ICompressSetEncoderProperties2> aSetEncoderProperties2;
        if (aMethodFull.MethodInfoEx.IsSimpleCoder())
        {
          RETURN_IF_NOT_S_OK(anEncoder->QueryInterface(&aSetEncoderProperties2));
        }
        else
        {
          RETURN_IF_NOT_S_OK(anEncoder2->QueryInterface(&aSetEncoderProperties2));
        }
        
        RETURN_IF_NOT_S_OK(aSetEncoderProperties2->SetEncoderProperties2(&aPropIDs.front(),
          &aProperties.front(), aNumProperties));
      }
      
      if (aMethodFull.CoderProperties.Size() > 0)
      {
        std::vector<NWindows::NCOM::CPropVariant> aProperties;
        std::vector<PROPID> aPropIDs;
        INT32 aNumProperties = aMethodFull.CoderProperties.Size();
        for (int i = 0; i < aNumProperties; i++)
        {
          const CProperty &aProperty = aMethodFull.CoderProperties[i];
          aPropIDs.push_back(aProperty.PropID);
          aProperties.push_back(aProperty.Value);
        }
        CComPtr<ICompressSetCoderProperties2> aSetCoderProperties2;
        if (aMethodFull.MethodInfoEx.IsSimpleCoder())
        {
          RETURN_IF_NOT_S_OK(anEncoder->QueryInterface(&aSetCoderProperties2));
        }
        else
        {
          RETURN_IF_NOT_S_OK(anEncoder2->QueryInterface(&aSetCoderProperties2));
        }
        
        RETURN_IF_NOT_S_OK(aSetCoderProperties2->SetCoderProperties2(&aPropIDs.front(),
          &aProperties.front(), aNumProperties));
      }

      CComPtr<ICompressWriteCoderProperties> aWriteCoderProperties;
        
      if (aMethodFull.MethodInfoEx.IsSimpleCoder())
      {
        anEncoder->QueryInterface(&aWriteCoderProperties);
      }
      else
      {
        anEncoder2->QueryInterface(&aWriteCoderProperties);
      }
        
      if (aWriteCoderProperties != NULL)
      {
        CComObjectNoLock<CSequentialOutStreamImp> *anOutStreamSpec = new 
          CComObjectNoLock<CSequentialOutStreamImp>;
        CComPtr<ISequentialOutStream> anOutStream(anOutStreamSpec);
        anOutStreamSpec->Init();
        aWriteCoderProperties->WriteCoderProperties(anOutStream);
        UINT32 aSize = anOutStreamSpec->GetSize();
        anEncodingInfo.Properties.SetCapacity(aSize);
        memmove(anEncodingInfo.Properties, anOutStreamSpec->GetBuffer(), aSize);
      }

      // public ICompressWriteCoderProperties,
      if (aMethodFull.MethodInfoEx.IsSimpleCoder())
      {
        m_Encoders.Add(anEncoder);
        aMixerCoderSpec->AddCoder(anEncoder);
      }
      else
      {
        m_Encoders2.Add(anEncoder2);
        aMixerCoderSpec->AddCoder2(anEncoder2);
      }
    }
  }
  aMixerCoderSpec->ReInit();
  // aMixerCoderSpec->SetCoderInfo(0, NULL, NULL, aProgress);

  CObjectVector<CInOutTempBuffer> anInOutTempBuffers;
  CObjectVector<CComObjectNoLock<CSequentialOutTempBufferImp> *>aTempBufferSpecs;
  CObjectVector<CComPtr<ISequentialOutStream> > aTempBuffers;
  int aNumMethods = m_BindInfo.CodersInfo.Size();
  for (int i = 1; i < m_BindInfo.OutStreams.Size(); i++)
  {
    anInOutTempBuffers.Add(CInOutTempBuffer());
    anInOutTempBuffers.Back().Create();
    anInOutTempBuffers.Back().InitWriting();
  }
  for (i = 1; i < m_BindInfo.OutStreams.Size(); i++)
  {
    CComObjectNoLock<CSequentialOutTempBufferImp> *aTempBufferSpec = 
      new CComObjectNoLock<CSequentialOutTempBufferImp>;
    CComPtr<ISequentialOutStream> aTempBuffer = aTempBufferSpec;
    aTempBufferSpec->Init(&anInOutTempBuffers[i - 1]);
    aTempBuffers.Add(aTempBuffer);
    aTempBufferSpecs.Add(aTempBufferSpec);
  }

  for (i = 0; i < aNumMethods; i++)
    aMixerCoderSpec->SetCoderInfo(i, NULL, NULL);

  if (m_BindInfo.InStreams.IsEmpty())
    return E_FAIL;
  UINT32 aMainCoderIndex, aMainStreamIndex;
  m_BindInfo.FindInStream(m_BindInfo.InStreams[0], aMainCoderIndex, aMainStreamIndex);
  aMixerCoderSpec->SetProgressCoderIndex(aMainCoderIndex);
  if (anInStreamSize != NULL)
  {
    CRecordVector<const UINT64 *> aSizePointers;
    for (int i = 0; i < m_BindInfo.CodersInfo[aMainCoderIndex].NumInStreams; i++)
      if (i == aMainStreamIndex)
        aSizePointers.Add(anInStreamSize);
      else
        aSizePointers.Add(NULL);
    aMixerCoderSpec->SetCoderInfo(aMainCoderIndex, &aSizePointers.Front(), NULL);
  }

  
  // UINT64 anOutStreamStartPos;
  // RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &anOutStreamStartPos));
  
  CComObjectNoLock<CSequentialInStreamSizeCount2> *anInStreamSizeCountSpec = 
      new CComObjectNoLock<CSequentialInStreamSizeCount2>;
  CComPtr<ISequentialInStream> anInStreamSizeCount = anInStreamSizeCountSpec;
  CComObjectNoLock<CSequentialOutStreamSizeCount> *anOutStreamSizeCountSpec = 
      new CComObjectNoLock<CSequentialOutStreamSizeCount>;
  CComPtr<ISequentialOutStream> anOutStreamSizeCount = anOutStreamSizeCountSpec;

  anInStreamSizeCountSpec->Init(anInStream);
  anOutStreamSizeCountSpec->Init(anOutStream);

  CRecordVector<ISequentialInStream *> anInStreamPointers;
  CRecordVector<ISequentialOutStream *> anOutStreamPointers;
  anInStreamPointers.Add(anInStreamSizeCount);
  anOutStreamPointers.Add(anOutStreamSizeCount);
  for (i = 1; i < m_BindInfo.OutStreams.Size(); i++)
    anOutStreamPointers.Add(aTempBuffers[i - 1]);
  
  RETURN_IF_NOT_S_OK(aMixerCoder->Code(&anInStreamPointers.Front(), NULL, 1,
    &anOutStreamPointers.Front(), NULL, anOutStreamPointers.Size(), aCompressProgress));
  
  ConvertBindInfoToFolderItemInfo(m_DecompressBindInfo, m_DecompressionMethods,
      aFolderItem);
  
  aPackSizes.Add(anOutStreamSizeCountSpec->GetSize());
  
  for (i = 1; i < m_BindInfo.OutStreams.Size(); i++)
  {
    CInOutTempBuffer &anInOutTempBuffer = anInOutTempBuffers[i - 1];
    anInOutTempBuffer.FlushWrite();
    anInOutTempBuffer.InitReading();
    anInOutTempBuffer.WriteToStream(anOutStream);
    aPackSizes.Add(anInOutTempBuffer.GetDataSize());
  }
  
  for (i = 0; i < m_BindReverseConverter->m_NumSrcInStreams; i++)
  {
    int aBinder = m_BindInfo.FindBinderForInStream(
        m_BindReverseConverter->m_DestOutToSrcInMap[i]);
    UINT64 aStreamSize;
    if (aBinder < 0)
      aStreamSize = anInStreamSizeCountSpec->GetSize();
    else
      aStreamSize = aMixerCoderSpec->GetWriteProcessedSize(aBinder);
    aFolderItem.UnPackSizes.Add(aStreamSize);
  }
  for (i = aNumMethods - 1; i >= 0; i--)
    aFolderItem.CodersInfo[aNumMethods - 1 - i].Properties = m_CodersInfo[i].Properties;
  return S_OK;
}

CEncoder::CEncoder(const CCompressionMethodMode *anOptions)
{
  m_Options = *anOptions;
  aMixerCoderSpec = NULL;

  for (int i = anOptions->Methods.Size() - 1; i >= 0; i--)
  {
    const CMethodFull &aMethodFull = anOptions->Methods[i];
    m_DecompressionMethods.Add(aMethodFull.MethodInfoEx.MethodID);
  }

  UINT32 aNumInStreams = 0;
  UINT32 aNumOutStreams = 0;
  for (i = 0; i < anOptions->Methods.Size(); i++)
  {
    const CMethodFull &aMethodFull = anOptions->Methods[i];
    NCoderMixer2::CCoderStreamsInfo aCoderStreamsInfo;
    aCoderStreamsInfo.NumInStreams = aMethodFull.MethodInfoEx.NumOutStreams;
    aCoderStreamsInfo.NumOutStreams = aMethodFull.MethodInfoEx.NumInStreams;
    if (anOptions->m_Binds.IsEmpty())
    {
      if (i < anOptions->Methods.Size() - 1)
      {
        NCoderMixer2::CBindPair aBindPair;
        aBindPair.InIndex = aNumInStreams + aCoderStreamsInfo.NumInStreams;
        aBindPair.OutIndex = aNumOutStreams;
        m_BindInfo.BindPairs.Add(aBindPair);
      }
      else
        m_BindInfo.OutStreams.Insert(0, aNumOutStreams);
      for (UINT32 j = 1; j < aCoderStreamsInfo.NumOutStreams; j++)
        m_BindInfo.OutStreams.Add(aNumOutStreams + j);
    }

    
    aNumInStreams += aCoderStreamsInfo.NumInStreams;
    aNumOutStreams += aCoderStreamsInfo.NumOutStreams;

    m_BindInfo.CodersInfo.Add(aCoderStreamsInfo);
  }

  if (!anOptions->m_Binds.IsEmpty())
  {
    for (int i = 0; i < anOptions->m_Binds.Size(); i++)
    {
      NCoderMixer2::CBindPair aBindPair;
      const CBind &aBind = anOptions->m_Binds[i];
      aBindPair.InIndex = m_BindInfo.GetCoderInStreamIndex(aBind.InCoder) + aBind.InStream;
      aBindPair.OutIndex = m_BindInfo.GetCoderOutStreamIndex(aBind.OutCoder) + aBind.OutStream;
      m_BindInfo.BindPairs.Add(aBindPair);
    }
    for (i = 0; i < aNumOutStreams; i++)
      if (m_BindInfo.FindBinderForOutStream(i) == -1)
        m_BindInfo.OutStreams.Add(i);
  }

  for (i = 0; i < aNumInStreams; i++)
    if (m_BindInfo.FindBinderForInStream(i) == -1)
      m_BindInfo.InStreams.Add(i);

  if (m_BindInfo.InStreams.IsEmpty())
    return; // this is error

  // Make main stream first in list
  int anInIndex = m_BindInfo.InStreams[0];
  while (true)
  {
    UINT32 aCoderIndex, aCoderStreamIndex;
    m_BindInfo.FindInStream(anInIndex, aCoderIndex, aCoderStreamIndex);
    UINT32 anOutIndex = m_BindInfo.GetCoderStartOutStream(aCoderIndex);
    int aBinder = m_BindInfo.FindBinderForOutStream(anOutIndex);
    if (aBinder >= 0)
    {
      anInIndex = m_BindInfo.BindPairs[aBinder].InIndex;
      continue;
    }
    for (i = 0; i < m_BindInfo.OutStreams.Size(); i++)
      if (m_BindInfo.OutStreams[i] == anOutIndex)
      {
        m_BindInfo.OutStreams.Delete(i);
        m_BindInfo.OutStreams.Insert(0, anOutIndex);
        break;
      }
    break;
  }


  m_BindReverseConverter = new NCoderMixer2::CBindReverseConverter(m_BindInfo);
  m_BindReverseConverter->CreateReverseBindInfo(m_DecompressBindInfo);
}

}}
