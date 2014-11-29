// 7zDecode.cpp

#include "StdAfx.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/LockedStream.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamObjects.h"

#include "7zDecode.h"

namespace NArchive {
namespace N7z {

static void ConvertFolderItemInfoToBindInfo(const CFolder &folder,
    CBindInfoEx &bindInfo)
{
  bindInfo.Clear();
  bindInfo.BindPairs.ClearAndSetSize(folder.BindPairs.Size());
  unsigned i;
  for (i = 0; i < folder.BindPairs.Size(); i++)
  {
    NCoderMixer::CBindPair &bindPair = bindInfo.BindPairs[i];
    bindPair.InIndex = (UInt32)folder.BindPairs[i].InIndex;
    bindPair.OutIndex = (UInt32)folder.BindPairs[i].OutIndex;
  }

  bindInfo.Coders.ClearAndSetSize(folder.Coders.Size());
  bindInfo.CoderMethodIDs.ClearAndSetSize(folder.Coders.Size());

  UInt32 outStreamIndex = 0;
  for (i = 0; i < folder.Coders.Size(); i++)
  {
    NCoderMixer::CCoderStreamsInfo &coderStreamsInfo = bindInfo.Coders[i];
    const CCoderInfo &coderInfo = folder.Coders[i];
    coderStreamsInfo.NumInStreams = (UInt32)coderInfo.NumInStreams;
    coderStreamsInfo.NumOutStreams = (UInt32)coderInfo.NumOutStreams;
    bindInfo.CoderMethodIDs[i] = coderInfo.MethodID;
    for (UInt32 j = 0; j < coderStreamsInfo.NumOutStreams; j++, outStreamIndex++)
      if (folder.FindBindPairForOutStream(outStreamIndex) < 0)
        bindInfo.OutStreams.Add(outStreamIndex);
  }
  bindInfo.InStreams.ClearAndSetSize(folder.PackStreams.Size());
  for (i = 0; i < folder.PackStreams.Size(); i++)
    bindInfo.InStreams[i] = (UInt32)folder.PackStreams[i];
}

static bool AreCodersEqual(const NCoderMixer::CCoderStreamsInfo &a1,
    const NCoderMixer::CCoderStreamsInfo &a2)
{
  return (a1.NumInStreams == a2.NumInStreams) &&
    (a1.NumOutStreams == a2.NumOutStreams);
}

static bool AreBindPairsEqual(const NCoderMixer::CBindPair &a1, const NCoderMixer::CBindPair &a2)
{
  return (a1.InIndex == a2.InIndex) &&
    (a1.OutIndex == a2.OutIndex);
}

static bool AreBindInfoExEqual(const CBindInfoEx &a1, const CBindInfoEx &a2)
{
  if (a1.Coders.Size() != a2.Coders.Size())
    return false;
  unsigned i;
  for (i = 0; i < a1.Coders.Size(); i++)
    if (!AreCodersEqual(a1.Coders[i], a2.Coders[i]))
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

CDecoder::CDecoder(bool multiThread)
{
  #ifndef _ST_MODE
  multiThread = true;
  #endif
  _multiThread = multiThread;
  _bindInfoExPrevIsDefined = false;
}

HRESULT CDecoder::Decode(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream,
    UInt64 startPos,
    const CFolders &folders, int folderIndex,
    ISequentialOutStream *outStream,
    ICompressProgressInfo *compressProgress
    _7Z_DECODER_CRYPRO_VARS_DECL
    #if !defined(_7ZIP_ST) && !defined(_SFX)
    , bool mtMode, UInt32 numThreads
    #endif
    )
{
  const UInt64 *packPositions = &folders.PackPositions[folders.FoStartPackStreamIndex[folderIndex]];
  CFolder folderInfo;
  folders.ParseFolderInfo(folderIndex, folderInfo);

  if (!folderInfo.CheckStructure(folders.GetNumFolderUnpackSizes(folderIndex)))
    return E_NOTIMPL;
  
  /*
  We don't need to init isEncrypted and passwordIsDefined
  We must upgrade them only
  #ifndef _NO_CRYPTO
  isEncrypted = false;
  passwordIsDefined = false;
  #endif
  */

  CObjectVector< CMyComPtr<ISequentialInStream> > inStreams;
  
  CLockedInStream lockedInStream;
  lockedInStream.Init(inStream);
  
  for (unsigned j = 0; j < folderInfo.PackStreams.Size(); j++)
  {
    CLockedSequentialInStreamImp *lockedStreamImpSpec = new CLockedSequentialInStreamImp;
    CMyComPtr<ISequentialInStream> lockedStreamImp = lockedStreamImpSpec;
    lockedStreamImpSpec->Init(&lockedInStream, startPos + packPositions[j]);
    CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
    CMyComPtr<ISequentialInStream> inStream = streamSpec;
    streamSpec->SetStream(lockedStreamImp);
    streamSpec->Init(packPositions[j + 1] - packPositions[j]);
    inStreams.Add(inStream);
  }
  
  unsigned numCoders = folderInfo.Coders.Size();
  
  CBindInfoEx bindInfo;
  ConvertFolderItemInfoToBindInfo(folderInfo, bindInfo);
  bool createNewCoders;
  if (!_bindInfoExPrevIsDefined)
    createNewCoders = true;
  else
    createNewCoders = !AreBindInfoExEqual(bindInfo, _bindInfoExPrev);
  if (createNewCoders)
  {
    unsigned i;
    _decoders.Clear();
    // _decoders2.Clear();
    
    _mixerCoder.Release();

    if (_multiThread)
    {
      _mixerCoderMTSpec = new NCoderMixer::CCoderMixer2MT;
      _mixerCoder = _mixerCoderMTSpec;
      _mixerCoderCommon = _mixerCoderMTSpec;
    }
    else
    {
      #ifdef _ST_MODE
      _mixerCoderSTSpec = new NCoderMixer::CCoderMixer2ST;
      _mixerCoder = _mixerCoderSTSpec;
      _mixerCoderCommon = _mixerCoderSTSpec;
      #endif
    }
    RINOK(_mixerCoderCommon->SetBindInfo(bindInfo));
    
    for (i = 0; i < numCoders; i++)
    {
      const CCoderInfo &coderInfo = folderInfo.Coders[i];

  
      CMyComPtr<ICompressCoder> decoder;
      CMyComPtr<ICompressCoder2> decoder2;
      RINOK(CreateCoder(
          EXTERNAL_CODECS_LOC_VARS
          coderInfo.MethodID, decoder, decoder2, false));
      CMyComPtr<IUnknown> decoderUnknown;
      if (coderInfo.IsSimpleCoder())
      {
        if (decoder == 0)
          return E_NOTIMPL;

        decoderUnknown = (IUnknown *)decoder;
        
        if (_multiThread)
          _mixerCoderMTSpec->AddCoder(decoder);
        #ifdef _ST_MODE
        else
          _mixerCoderSTSpec->AddCoder(decoder, false);
        #endif
      }
      else
      {
        if (decoder2 == 0)
          return E_NOTIMPL;
        decoderUnknown = (IUnknown *)decoder2;
        if (_multiThread)
          _mixerCoderMTSpec->AddCoder2(decoder2);
        #ifdef _ST_MODE
        else
          _mixerCoderSTSpec->AddCoder2(decoder2, false);
        #endif
      }
      _decoders.Add(decoderUnknown);
      #ifdef EXTERNAL_CODECS
      CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
      decoderUnknown.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
      if (setCompressCodecsInfo)
      {
        RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(__externalCodecs->GetCodecs));
      }
      #endif
    }
    _bindInfoExPrev = bindInfo;
    _bindInfoExPrevIsDefined = true;
  }
  unsigned i;
  _mixerCoderCommon->ReInit();
  
  UInt32 packStreamIndex = 0;
  UInt32 unpackStreamIndexStart = folders.FoToCoderUnpackSizes[folderIndex];
  UInt32 unpackStreamIndex = unpackStreamIndexStart;
  UInt32 coderIndex = 0;
  // UInt32 coder2Index = 0;
  
  for (i = 0; i < numCoders; i++)
  {
    const CCoderInfo &coderInfo = folderInfo.Coders[i];
    CMyComPtr<IUnknown> &decoder = _decoders[coderIndex];
    
    {
      CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
      decoder.QueryInterface(IID_ICompressSetDecoderProperties2, &setDecoderProperties);
      if (setDecoderProperties)
      {
        const CByteBuffer &props = coderInfo.Props;
        size_t size = props.Size();
        if (size > 0xFFFFFFFF)
          return E_NOTIMPL;
        // if (size > 0)
        {
          RINOK(setDecoderProperties->SetDecoderProperties2((const Byte *)props, (UInt32)size));
        }
      }
    }

    #if !defined(_7ZIP_ST) && !defined(_SFX)
    if (mtMode)
    {
      CMyComPtr<ICompressSetCoderMt> setCoderMt;
      decoder.QueryInterface(IID_ICompressSetCoderMt, &setCoderMt);
      if (setCoderMt)
      {
        RINOK(setCoderMt->SetNumberOfThreads(numThreads));
      }
    }
    #endif

    #ifndef _NO_CRYPTO
    {
      CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
      decoder.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword);
      if (cryptoSetPassword)
      {
        isEncrypted = true;
        if (!getTextPassword)
          return E_NOTIMPL;
        CMyComBSTR passwordBSTR;
        RINOK(getTextPassword->CryptoGetTextPassword(&passwordBSTR));
        passwordIsDefined = true;
        size_t len = 0;
        if (passwordBSTR)
          len = MyStringLen((BSTR)passwordBSTR);
        CByteBuffer buffer(len * 2);
        for (size_t i = 0; i < len; i++)
        {
          wchar_t c = passwordBSTR[i];
          ((Byte *)buffer)[i * 2] = (Byte)c;
          ((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
        }
        RINOK(cryptoSetPassword->CryptoSetPassword((const Byte *)buffer, (UInt32)buffer.Size()));
      }
    }
    #endif

    coderIndex++;
    
    UInt32 numInStreams = (UInt32)coderInfo.NumInStreams;
    UInt32 numOutStreams = (UInt32)coderInfo.NumOutStreams;
    CObjArray<UInt64> packSizes(numInStreams);
    CObjArray<const UInt64 *> packSizesPointers(numInStreams);
    CObjArray<const UInt64 *> unpackSizesPointers(numOutStreams);
    UInt32 j;

    for (j = 0; j < numOutStreams; j++, unpackStreamIndex++)
      unpackSizesPointers[j] = &folders.CoderUnpackSizes[unpackStreamIndex];
    
    for (j = 0; j < numInStreams; j++, packStreamIndex++)
    {
      int bindPairIndex = folderInfo.FindBindPairForInStream(packStreamIndex);
      if (bindPairIndex >= 0)
        packSizesPointers[j] = &folders.CoderUnpackSizes[unpackStreamIndexStart + (UInt32)folderInfo.BindPairs[bindPairIndex].OutIndex];
      else
      {
        int index = folderInfo.FindPackStreamArrayIndex(packStreamIndex);
        if (index < 0)
          return S_FALSE; // check it
        packSizes[j] = packPositions[index + 1] - packPositions[index];
        packSizesPointers[j] = &packSizes[j];
      }
    }
    
    _mixerCoderCommon->SetCoderInfo(i, packSizesPointers, unpackSizesPointers);
  }
  UInt32 mainCoder, temp;
  bindInfo.FindOutStream(bindInfo.OutStreams[0], mainCoder, temp);

  if (_multiThread)
    _mixerCoderMTSpec->SetProgressCoderIndex(mainCoder);
  /*
  else
    _mixerCoderSTSpec->SetProgressCoderIndex(mainCoder);;
  */
  
  if (numCoders == 0)
    return 0;
  unsigned num = inStreams.Size();
  CObjArray<ISequentialInStream *> inStreamPointers(num);
  for (i = 0; i < num; i++)
    inStreamPointers[i] = inStreams[i];
  ISequentialOutStream *outStreamPointer = outStream;
  return _mixerCoder->Code(
      inStreamPointers, NULL, num,
      &outStreamPointer, NULL, 1,
      compressProgress);
}

}}
