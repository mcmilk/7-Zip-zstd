// CoderMixer2ST.cpp

#include "StdAfx.h"

#include "CoderMixer2ST.h"

namespace NCoderMixer2 {

CCoderMixer2ST::CCoderMixer2ST() {}

CCoderMixer2ST::~CCoderMixer2ST(){ }

HRESULT CCoderMixer2ST::SetBindInfo(const CBindInfo &bindInfo)
{
  _bindInfo = bindInfo;
  return S_OK;
}

void CCoderMixer2ST::AddCoderCommon(bool isMain)
{
  const CCoderStreamsInfo &csi = _bindInfo.Coders[_coders.Size()];
  _coders.Add(CSTCoderInfo(csi.NumInStreams, csi.NumOutStreams, isMain));
}

void CCoderMixer2ST::AddCoder(ICompressCoder *coder, bool isMain)
{
  AddCoderCommon(isMain);
  _coders.Back().Coder = coder;
}

void CCoderMixer2ST::AddCoder2(ICompressCoder2 *coder, bool isMain)
{
  AddCoderCommon(isMain);
  _coders.Back().Coder2 = coder;
}

void CCoderMixer2ST::ReInit() { }

HRESULT CCoderMixer2ST::GetInStream(
    ISequentialInStream **inStreams, const UInt64 **inSizes,
    UInt32 streamIndex, ISequentialInStream **inStreamRes)
{
  CMyComPtr<ISequentialInStream> seqInStream;
  int i;
  for(i = 0; i < _bindInfo.InStreams.Size(); i++)
    if (_bindInfo.InStreams[i] == streamIndex)
    {
      seqInStream = inStreams[i];
      *inStreamRes = seqInStream.Detach();
      return  S_OK;
    }
  int binderIndex = _bindInfo.FindBinderForInStream(streamIndex);
  if (binderIndex < 0)
    return E_INVALIDARG;

  UInt32 coderIndex, coderStreamIndex;
  _bindInfo.FindOutStream(_bindInfo.BindPairs[binderIndex].OutIndex,
      coderIndex, coderStreamIndex);

  CCoderInfo &coder = _coders[coderIndex];
  if (!coder.Coder)
    return E_NOTIMPL;
  coder.Coder.QueryInterface(IID_ISequentialInStream, &seqInStream);
  if (!seqInStream)
    return E_NOTIMPL;

  UInt32 startIndex = _bindInfo.GetCoderInStreamIndex(coderIndex);

  CMyComPtr<ICompressSetInStream> setInStream;
  if (!coder.Coder)
    return E_NOTIMPL;
  coder.Coder.QueryInterface(IID_ICompressSetInStream, &setInStream);
  if (!setInStream)
    return E_NOTIMPL;

  if (coder.NumInStreams > 1)
    return E_NOTIMPL;
  for (i = 0; i < (int)coder.NumInStreams; i++)
  {
    CMyComPtr<ISequentialInStream> seqInStream2;
    RINOK(GetInStream(inStreams, inSizes, startIndex + i, &seqInStream2));
    RINOK(setInStream->SetInStream(seqInStream2));
  }
  *inStreamRes = seqInStream.Detach();
  return S_OK;
}

HRESULT CCoderMixer2ST::GetOutStream(
    ISequentialOutStream **outStreams, const UInt64 **outSizes,
    UInt32 streamIndex, ISequentialOutStream **outStreamRes)
{
  CMyComPtr<ISequentialOutStream> seqOutStream;
  int i;
  for(i = 0; i < _bindInfo.OutStreams.Size(); i++)
    if (_bindInfo.OutStreams[i] == streamIndex)
    {
      seqOutStream = outStreams[i];
      *outStreamRes = seqOutStream.Detach();
      return  S_OK;
    }
  int binderIndex = _bindInfo.FindBinderForOutStream(streamIndex);
  if (binderIndex < 0)
    return E_INVALIDARG;

  UInt32 coderIndex, coderStreamIndex;
  _bindInfo.FindInStream(_bindInfo.BindPairs[binderIndex].InIndex,
      coderIndex, coderStreamIndex);

  CCoderInfo &coder = _coders[coderIndex];
  if (!coder.Coder)
    return E_NOTIMPL;
  coder.Coder.QueryInterface(IID_ISequentialOutStream, &seqOutStream);
  if (!seqOutStream)
    return E_NOTIMPL;

  UInt32 startIndex = _bindInfo.GetCoderOutStreamIndex(coderIndex);

  CMyComPtr<ICompressSetOutStream> setOutStream;
  if (!coder.Coder)
    return E_NOTIMPL;
  coder.Coder.QueryInterface(IID_ICompressSetOutStream, &setOutStream);
  if (!setOutStream)
    return E_NOTIMPL;

  if (coder.NumOutStreams > 1)
    return E_NOTIMPL;
  for (i = 0; i < (int)coder.NumOutStreams; i++)
  {
    CMyComPtr<ISequentialOutStream> seqOutStream2;
    RINOK(GetOutStream(outStreams, outSizes, startIndex + i, &seqOutStream2));
    RINOK(setOutStream->SetOutStream(seqOutStream2));
  }
  *outStreamRes = seqOutStream.Detach();
  return S_OK;
}
    

STDMETHODIMP CCoderMixer2ST::Code(ISequentialInStream **inStreams,
      const UInt64 **inSizes,
      UInt32 numInStreams,
      ISequentialOutStream **outStreams,
      const UInt64 **outSizes,
      UInt32 numOutStreams,
      ICompressProgressInfo *progress)
{
  if (numInStreams != (UInt32)_bindInfo.InStreams.Size() ||
      numOutStreams != (UInt32)_bindInfo.OutStreams.Size())
    return E_INVALIDARG;

  // Find main coder
  int _mainCoderIndex = -1;
  int i;
  for (i = 0; i < _coders.Size(); i++)
    if (_coders[i].IsMain)
    {
      _mainCoderIndex = i;
      break;
    }
  if (_mainCoderIndex < 0)
  for (i = 0; i < _coders.Size(); i++)
    if (_coders[i].NumInStreams > 1)
    {
      if (_mainCoderIndex >= 0)
        return E_NOTIMPL;
      _mainCoderIndex = i;
    }
  if (_mainCoderIndex < 0)
    _mainCoderIndex = 0;
 
  // _mainCoderIndex = 0;
  // _mainCoderIndex = _coders.Size() - 1;
  CCoderInfo &mainCoder = _coders[_mainCoderIndex];

  CObjectVector< CMyComPtr<ISequentialInStream> > seqInStreams;
  CObjectVector< CMyComPtr<ISequentialOutStream> > seqOutStreams;
  UInt32 startInIndex = _bindInfo.GetCoderInStreamIndex(_mainCoderIndex);
  UInt32 startOutIndex = _bindInfo.GetCoderOutStreamIndex(_mainCoderIndex);
  for (i = 0; i < (int)mainCoder.NumInStreams; i++)
  {
    CMyComPtr<ISequentialInStream> seqInStream;
    RINOK(GetInStream(inStreams, inSizes, startInIndex + i, &seqInStream));
    seqInStreams.Add(seqInStream);
  }
  for (i = 0; i < (int)mainCoder.NumOutStreams; i++)
  {
    CMyComPtr<ISequentialOutStream> seqOutStream;
    RINOK(GetOutStream(outStreams, outSizes, startOutIndex + i, &seqOutStream));
    seqOutStreams.Add(seqOutStream);
  }
  CRecordVector< ISequentialInStream * > seqInStreamsSpec;
  CRecordVector< ISequentialOutStream * > seqOutStreamsSpec;
  for (i = 0; i < (int)mainCoder.NumInStreams; i++)
    seqInStreamsSpec.Add(seqInStreams[i]);
  for (i = 0; i < (int)mainCoder.NumOutStreams; i++)
    seqOutStreamsSpec.Add(seqOutStreams[i]);

  for (i = 0; i < _coders.Size(); i++)
  {
    if (i == _mainCoderIndex)
      continue;
    CCoderInfo &coder = _coders[i];
    CMyComPtr<ICompressSetOutStreamSize> setOutStreamSize;
    coder.Coder.QueryInterface(IID_ICompressSetOutStreamSize, &setOutStreamSize);
    if (setOutStreamSize)
    {
      RINOK(setOutStreamSize->SetOutStreamSize(coder.OutSizePointers[0]));
    }
  }
  if (mainCoder.Coder)
  {
    RINOK(mainCoder.Coder->Code(
        seqInStreamsSpec[0], seqOutStreamsSpec[0],
        mainCoder.InSizePointers[0], mainCoder.OutSizePointers[0],
        progress));
  }
  else
  {
    RINOK(mainCoder.Coder2->Code(
        &seqInStreamsSpec.Front(),
        &mainCoder.InSizePointers.Front(), mainCoder.NumInStreams,
        &seqOutStreamsSpec.Front(),
        &mainCoder.OutSizePointers.Front(), mainCoder.NumOutStreams,
        progress));
  }
  CMyComPtr<IOutStreamFlush> flush;
  seqOutStreams.Front().QueryInterface(IID_IOutStreamFlush, &flush);
  if (flush)
    return flush->Flush();
  return S_OK;
}

/*
UInt64 CCoderMixer2ST::GetWriteProcessedSize(UInt32 binderIndex) const
{
  return _streamBinders[binderIndex].ProcessedSize;
}
*/

}
