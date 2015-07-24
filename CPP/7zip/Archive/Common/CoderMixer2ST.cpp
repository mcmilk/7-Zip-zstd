// CoderMixer2ST.cpp

#include "StdAfx.h"

#include "CoderMixer2ST.h"

STDMETHODIMP CSequentialInStreamCalcSize::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessed = 0;
  HRESULT result = S_OK;
  if (_stream)
    result = _stream->Read(data, size, &realProcessed);
  _size += realProcessed;
  if (size != 0 && realProcessed == 0)
    _wasFinished = true;
  if (processedSize)
    *processedSize = realProcessed;
  return result;
}



STDMETHODIMP COutStreamCalcSize::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  HRESULT result = S_OK;
  if (_stream)
    result = _stream->Write(data, size, &size);
  _size += size;
  if (processedSize)
    *processedSize = size;
  return result;
}

STDMETHODIMP COutStreamCalcSize::Flush()
{
  HRESULT result = S_OK;
  if (_stream)
  {
    CMyComPtr<IOutStreamFlush> outStreamFlush;
    _stream.QueryInterface(IID_IOutStreamFlush, &outStreamFlush);
    if (outStreamFlush)
      result = outStreamFlush->Flush();;
  }
  return result;
}



namespace NCoderMixer2 {

CMixerST::CMixerST(bool encodeMode):
    CMixer(encodeMode)
    {}

CMixerST::~CMixerST() {}

void CMixerST::AddCoder(ICompressCoder *coder, ICompressCoder2 *coder2, bool isFilter)
{
  IsFilter_Vector.Add(isFilter);
  const CCoderStreamsInfo &c = _bi.Coders[_coders.Size()];
  CCoderST &c2 = _coders.AddNew();
  c2.NumStreams = c.NumStreams;
  c2.Coder = coder;
  c2.Coder2 = coder2;

  /*
  if (isFilter)
  {
    c2.CanRead = true;
    c2.CanWrite = true;
  }
  else
  */
  {
    IUnknown *unk = (coder ? (IUnknown *)coder : (IUnknown *)coder2);
    {
      CMyComPtr<ISequentialInStream> s;
      unk->QueryInterface(IID_ISequentialInStream, (void**)&s);
      c2.CanRead = (s != NULL);
    }
    {
      CMyComPtr<ISequentialOutStream> s;
      unk->QueryInterface(IID_ISequentialOutStream, (void**)&s);
      c2.CanWrite = (s != NULL);
    }
  }
}

CCoder &CMixerST::GetCoder(unsigned index)
{
  return _coders[index];
}

void CMixerST::ReInit() {}

HRESULT CMixerST::GetInStream2(
    ISequentialInStream * const *inStreams, /* const UInt64 * const *inSizes, */
    UInt32 outStreamIndex, ISequentialInStream **inStreamRes)
{
  UInt32 coderIndex = outStreamIndex, coderStreamIndex = 0;

  if (EncodeMode)
  {
    _bi.GetCoder_for_Stream(outStreamIndex, coderIndex, coderStreamIndex);
    if (coderStreamIndex != 0)
      return E_NOTIMPL;
  }

  const CCoder &coder = _coders[coderIndex];
  
  CMyComPtr<ISequentialInStream> seqInStream;
  coder.QueryInterface(IID_ISequentialInStream, (void **)&seqInStream);
  if (!seqInStream)
    return E_NOTIMPL;

  UInt32 numInStreams = EncodeMode ? 1 : coder.NumStreams;
  UInt32 startIndex = EncodeMode ? coderIndex : _bi.Coder_to_Stream[coderIndex];

  bool isSet = false;
  
  if (numInStreams == 1)
  {
    CMyComPtr<ICompressSetInStream> setStream;
    coder.QueryInterface(IID_ICompressSetInStream, (void **)&setStream);
    if (setStream)
    {
      CMyComPtr<ISequentialInStream> seqInStream2;
      RINOK(GetInStream(inStreams, /* inSizes, */ startIndex + 0, &seqInStream2));
      RINOK(setStream->SetInStream(seqInStream2));
      isSet = true;
    }
  }
  
  if (!isSet && numInStreams != 0)
  {
    CMyComPtr<ICompressSetInStream2> setStream2;
    coder.QueryInterface(IID_ICompressSetInStream2, (void **)&setStream2);
    if (!setStream2)
      return E_NOTIMPL;
    
    for (UInt32 i = 0; i < numInStreams; i++)
    {
      CMyComPtr<ISequentialInStream> seqInStream2;
      RINOK(GetInStream(inStreams, /* inSizes, */ startIndex + i, &seqInStream2));
      RINOK(setStream2->SetInStream2(i, seqInStream2));
    }
  }

  *inStreamRes = seqInStream.Detach();
  return S_OK;
}


HRESULT CMixerST::GetInStream(
    ISequentialInStream * const *inStreams, /* const UInt64 * const *inSizes, */
    UInt32 inStreamIndex, ISequentialInStream **inStreamRes)
{
  CMyComPtr<ISequentialInStream> seqInStream;
  
  {
    int index = -1;
    if (EncodeMode)
    {
      if (_bi.UnpackCoder == inStreamIndex)
        index = 0;
    }
    else
      index = _bi.FindStream_in_PackStreams(inStreamIndex);

    if (index >= 0)
    {
      seqInStream = inStreams[index];
      *inStreamRes = seqInStream.Detach();
      return S_OK;
    }
  }
  
  int bond = FindBond_for_Stream(
      true, // forInputStream
      inStreamIndex);
  if (bond < 0)
    return E_INVALIDARG;

  RINOK(GetInStream2(inStreams, /* inSizes, */
      _bi.Bonds[bond].Get_OutIndex(EncodeMode), &seqInStream));

  while (_binderStreams.Size() <= (unsigned)bond)
    _binderStreams.AddNew();
  CStBinderStream &bs = _binderStreams[bond];

  if (bs.StreamRef || bs.InStreamSpec)
    return E_NOTIMPL;
  
  CSequentialInStreamCalcSize *spec = new CSequentialInStreamCalcSize;
  bs.StreamRef = spec;
  bs.InStreamSpec = spec;
  
  spec->SetStream(seqInStream);
  spec->Init();
  
  seqInStream = bs.InStreamSpec;

  *inStreamRes = seqInStream.Detach();
  return S_OK;
}


HRESULT CMixerST::GetOutStream(
    ISequentialOutStream * const *outStreams, /* const UInt64 * const *outSizes, */
    UInt32 outStreamIndex, ISequentialOutStream **outStreamRes)
{
  CMyComPtr<ISequentialOutStream> seqOutStream;
  
  {
    int index = -1;
    if (!EncodeMode)
    {
      if (_bi.UnpackCoder == outStreamIndex)
        index = 0;
    }
    else
      index = _bi.FindStream_in_PackStreams(outStreamIndex);

    if (index >= 0)
    {
      seqOutStream = outStreams[index];
      *outStreamRes = seqOutStream.Detach();
      return S_OK;
    }
  }
  
  int bond = FindBond_for_Stream(
      false, // forInputStream
      outStreamIndex);
  if (bond < 0)
    return E_INVALIDARG;

  UInt32 inStreamIndex = _bi.Bonds[bond].Get_InIndex(EncodeMode);

  UInt32 coderIndex = inStreamIndex;
  UInt32 coderStreamIndex = 0;

  if (!EncodeMode)
    _bi.GetCoder_for_Stream(inStreamIndex, coderIndex, coderStreamIndex);

  CCoder &coder = _coders[coderIndex];

  /*
  if (!coder.Coder)
    return E_NOTIMPL;
  */

  coder.QueryInterface(IID_ISequentialOutStream, (void **)&seqOutStream);
  if (!seqOutStream)
    return E_NOTIMPL;

  UInt32 numOutStreams = EncodeMode ? coder.NumStreams : 1;
  UInt32 startIndex = EncodeMode ? _bi.Coder_to_Stream[coderIndex]: coderIndex;

  bool isSet = false;

  if (numOutStreams == 1)
  {
    CMyComPtr<ICompressSetOutStream> setOutStream;
    coder.Coder.QueryInterface(IID_ICompressSetOutStream, &setOutStream);
    if (setOutStream)
    {
      CMyComPtr<ISequentialOutStream> seqOutStream2;
      RINOK(GetOutStream(outStreams, /* outSizes, */ startIndex + 0, &seqOutStream2));
      RINOK(setOutStream->SetOutStream(seqOutStream2));
      isSet = true;
    }
  }

  if (!isSet && numOutStreams != 0)
  {
    // return E_NOTIMPL;
    // /*
    CMyComPtr<ICompressSetOutStream2> setStream2;
    coder.QueryInterface(IID_ICompressSetOutStream2, (void **)&setStream2);
    if (!setStream2)
      return E_NOTIMPL;
    for (UInt32 i = 0; i < numOutStreams; i++)
    {
      CMyComPtr<ISequentialOutStream> seqOutStream2;
      RINOK(GetOutStream(outStreams, startIndex + i, &seqOutStream2));
      RINOK(setStream2->SetOutStream2(i, seqOutStream2));
    }
    // */
  }

  while (_binderStreams.Size() <= (unsigned)bond)
    _binderStreams.AddNew();
  CStBinderStream &bs = _binderStreams[bond];

  if (bs.StreamRef || bs.OutStreamSpec)
    return E_NOTIMPL;
  
  COutStreamCalcSize *spec = new COutStreamCalcSize;
  bs.StreamRef = (ISequentialOutStream *)spec;
  bs.OutStreamSpec = spec;
  
  spec->SetStream(seqOutStream);
  spec->Init();

  seqOutStream = bs.OutStreamSpec;
  
  *outStreamRes = seqOutStream.Detach();
  return S_OK;
}


static HRESULT GetError(HRESULT res, HRESULT res2)
{
  if (res == res2)
    return res;
  if (res == S_OK)
    return res2;
  if (res == k_My_HRESULT_WritingWasCut)
  {
    if (res2 != S_OK)
      return res2;
  }
  return res;
}


HRESULT CMixerST::FlushStream(UInt32 streamIndex)
{
  {
    int index = -1;
    if (!EncodeMode)
    {
      if (_bi.UnpackCoder == streamIndex)
        index = 0;
    }
    else
      index = _bi.FindStream_in_PackStreams(streamIndex);

    if (index >= 0)
      return S_OK;
  }

  int bond = FindBond_for_Stream(
      false, // forInputStream
      streamIndex);
  if (bond < 0)
    return E_INVALIDARG;

  UInt32 inStreamIndex = _bi.Bonds[bond].Get_InIndex(EncodeMode);

  UInt32 coderIndex = inStreamIndex;
  UInt32 coderStreamIndex = 0;
  if (!EncodeMode)
    _bi.GetCoder_for_Stream(inStreamIndex, coderIndex, coderStreamIndex);

  CCoder &coder = _coders[coderIndex];
  CMyComPtr<IOutStreamFlush> flush;
  coder.QueryInterface(IID_IOutStreamFlush, (void **)&flush);
  HRESULT res = S_OK;
  if (flush)
  {
    res = flush->Flush();
  }
  return GetError(res, FlushCoder(coderIndex));
}


HRESULT CMixerST::FlushCoder(UInt32 coderIndex)
{
  CCoder &coder = _coders[coderIndex];

  UInt32 numOutStreams = EncodeMode ? coder.NumStreams : 1;
  UInt32 startIndex = EncodeMode ? _bi.Coder_to_Stream[coderIndex]: coderIndex;

  HRESULT res = S_OK;
  for (unsigned i = 0; i < numOutStreams; i++)
    res = GetError(res, FlushStream(startIndex + i));
  return res;
}


void CMixerST::SelectMainCoder(bool useFirst)
{
  unsigned ci = _bi.UnpackCoder;
  
  int firstNonFilter = -1;
  int firstAllowed = ci;
  
  for (;;)
  {
    const CCoderST &coder = _coders[ci];
    // break;
    
    if (ci != _bi.UnpackCoder)
      if (EncodeMode ? !coder.CanWrite : !coder.CanRead)
      {
        firstAllowed = ci;
        firstNonFilter = -2;
      }
      
      if (coder.NumStreams != 1)
        break;
      
      UInt32 st = _bi.Coder_to_Stream[ci];
      if (_bi.IsStream_in_PackStreams(st))
        break;
      int bond = _bi.FindBond_for_PackStream(st);
      if (bond < 0)
        throw 20150213;
      
      if (EncodeMode ? !coder.CanRead : !coder.CanWrite)
        break;
      
      if (firstNonFilter == -1 && !IsFilter_Vector[ci])
        firstNonFilter = ci;
      
      ci = _bi.Bonds[bond].UnpackIndex;
  }
  
  ci = firstNonFilter;
  if (firstNonFilter < 0 || useFirst)
    ci = firstAllowed;
  MainCoderIndex = ci;
}


HRESULT CMixerST::Code(
    ISequentialInStream * const *inStreams,
    ISequentialOutStream * const *outStreams,
    ICompressProgressInfo *progress)
{
  _binderStreams.Clear();
  unsigned ci = MainCoderIndex;
 
  const CCoder &mainCoder = _coders[MainCoderIndex];

  CObjectVector< CMyComPtr<ISequentialInStream> > seqInStreams;
  CObjectVector< CMyComPtr<ISequentialOutStream> > seqOutStreams;
  
  UInt32 numInStreams  =  EncodeMode ? 1 : mainCoder.NumStreams;
  UInt32 numOutStreams = !EncodeMode ? 1 : mainCoder.NumStreams;
  
  UInt32 startInIndex  =  EncodeMode ? ci : _bi.Coder_to_Stream[ci];
  UInt32 startOutIndex = !EncodeMode ? ci : _bi.Coder_to_Stream[ci];
  
  UInt32 i;

  for (i = 0; i < numInStreams; i++)
  {
    CMyComPtr<ISequentialInStream> seqInStream;
    RINOK(GetInStream(inStreams, /* inSizes, */ startInIndex + i, &seqInStream));
    seqInStreams.Add(seqInStream);
  }
  
  for (i = 0; i < numOutStreams; i++)
  {
    CMyComPtr<ISequentialOutStream> seqOutStream;
    RINOK(GetOutStream(outStreams, /* outSizes, */ startOutIndex + i, &seqOutStream));
    seqOutStreams.Add(seqOutStream);
  }
  
  CRecordVector< ISequentialInStream * > seqInStreamsSpec;
  CRecordVector< ISequentialOutStream * > seqOutStreamsSpec;
  
  for (i = 0; i < numInStreams; i++)
    seqInStreamsSpec.Add(seqInStreams[i]);
  for (i = 0; i < numOutStreams; i++)
    seqOutStreamsSpec.Add(seqOutStreams[i]);

  for (i = 0; i < _coders.Size(); i++)
  {
    if (i == ci)
      continue;
   
    CCoder &coder = _coders[i];

    CMyComPtr<ICompressSetOutStreamSize> setOutStreamSize;
    coder.QueryInterface(IID_ICompressSetOutStreamSize, (void **)&setOutStreamSize);
    if (setOutStreamSize)
    {
      RINOK(setOutStreamSize->SetOutStreamSize(
          EncodeMode ? coder.PackSizePointers[0] : coder.UnpackSizePointer));
    }
  }

  const UInt64 * const *isSizes2 = EncodeMode ? &mainCoder.UnpackSizePointer : &mainCoder.PackSizePointers.Front();
  const UInt64 * const *outSizes2 = EncodeMode ? &mainCoder.PackSizePointers.Front() : &mainCoder.UnpackSizePointer;

  HRESULT res;
  if (mainCoder.Coder)
  {
    res = mainCoder.Coder->Code(
        seqInStreamsSpec[0], seqOutStreamsSpec[0],
        isSizes2[0], outSizes2[0],
        progress);
  }
  else
  {
    res = mainCoder.Coder2->Code(
        &seqInStreamsSpec.Front(), isSizes2, numInStreams,
        &seqOutStreamsSpec.Front(), outSizes2, numOutStreams,
        progress);
  }

  if (res == k_My_HRESULT_WritingWasCut)
    res = S_OK;

  if (res == S_OK || res == S_FALSE)
  {
    res = GetError(res, FlushCoder(ci));
  }

  for (i = 0; i < _binderStreams.Size(); i++)
  {
    const CStBinderStream &bs = _binderStreams[i];
    if (bs.InStreamSpec)
      bs.InStreamSpec->ReleaseStream();
    else
      bs.OutStreamSpec->ReleaseStream();
  }

  if (res == k_My_HRESULT_WritingWasCut)
    res = S_OK;
  return res;
}


HRESULT CMixerST::GetMainUnpackStream(
    ISequentialInStream * const *inStreams,
    ISequentialInStream **inStreamRes)
{
  CMyComPtr<ISequentialInStream> seqInStream;

  RINOK(GetInStream2(inStreams, /* inSizes, */
      _bi.UnpackCoder, &seqInStream))
  
  FOR_VECTOR (i, _coders)
  {
    CCoder &coder = _coders[i];
    CMyComPtr<ICompressSetOutStreamSize> setOutStreamSize;
    coder.QueryInterface(IID_ICompressSetOutStreamSize, (void **)&setOutStreamSize);
    if (setOutStreamSize)
    {
      RINOK(setOutStreamSize->SetOutStreamSize(coder.UnpackSizePointer));
    }
  }
  
  *inStreamRes = seqInStream.Detach();
  return S_OK;
}


UInt64 CMixerST::GetBondStreamSize(unsigned bondIndex) const
{
  const CStBinderStream &bs = _binderStreams[bondIndex];
  if (bs.InStreamSpec)
    return bs.InStreamSpec->GetSize();
  return bs.OutStreamSpec->GetSize();
}

}
