// CoderMixer2MT.cpp

#include "StdAfx.h"

#include "CoderMixer2MT.h"

namespace NCoderMixer2 {

void CCoderMT::Execute() { Code(NULL); }

void CCoderMT::Code(ICompressProgressInfo *progress)
{
  unsigned numInStreams = EncodeMode ? 1 : NumStreams;
  unsigned numOutStreams = EncodeMode ? NumStreams : 1;

  InStreamPointers.ClearAndReserve(numInStreams);
  OutStreamPointers.ClearAndReserve(numOutStreams);

  unsigned i;
  
  for (i = 0; i < numInStreams; i++)
    InStreamPointers.AddInReserved((ISequentialInStream *)InStreams[i]);
  
  for (i = 0; i < numOutStreams; i++)
    OutStreamPointers.AddInReserved((ISequentialOutStream *)OutStreams[i]);

  // we suppose that UnpackSizePointer and PackSizePointers contain correct pointers.
  /*
  if (UnpackSizePointer)
    UnpackSizePointer = &UnpackSize;
  for (i = 0; i < NumStreams; i++)
    if (PackSizePointers[i])
      PackSizePointers[i] = &PackSizes[i];
  */

  if (Coder)
    Result = Coder->Code(InStreamPointers[0], OutStreamPointers[0],
        EncodeMode ? UnpackSizePointer : PackSizePointers[0],
        EncodeMode ? PackSizePointers[0] : UnpackSizePointer,
        progress);
  else
    Result = Coder2->Code(
        &InStreamPointers.Front(),  EncodeMode ? &UnpackSizePointer : &PackSizePointers.Front(), numInStreams,
        &OutStreamPointers.Front(), EncodeMode ? &PackSizePointers.Front(): &UnpackSizePointer, numOutStreams,
        progress);
  
  InStreamPointers.Clear();
  OutStreamPointers.Clear();

  for (i = 0; i < InStreams.Size(); i++)
    InStreams[i].Release();
  for (i = 0; i < OutStreams.Size(); i++)
    OutStreams[i].Release();
}

HRESULT CMixerMT::SetBindInfo(const CBindInfo &bindInfo)
{
  CMixer::SetBindInfo(bindInfo);
  
  _streamBinders.Clear();
  FOR_VECTOR (i, _bi.Bonds)
  {
    RINOK(_streamBinders.AddNew().CreateEvents());
  }
  return S_OK;
}

void CMixerMT::AddCoder(ICompressCoder *coder, ICompressCoder2 *coder2, bool isFilter)
{
  const CCoderStreamsInfo &c = _bi.Coders[_coders.Size()];
  CCoderMT &c2 = _coders.AddNew();
  c2.NumStreams = c.NumStreams;
  c2.EncodeMode = EncodeMode;
  c2.Coder = coder;
  c2.Coder2 = coder2;
  IsFilter_Vector.Add(isFilter);
}

CCoder &CMixerMT::GetCoder(unsigned index)
{
  return _coders[index];
}

void CMixerMT::ReInit()
{
  FOR_VECTOR (i, _streamBinders)
    _streamBinders[i].ReInit();
}

void CMixerMT::SelectMainCoder(bool useFirst)
{
  unsigned ci = _bi.UnpackCoder;

  if (!useFirst)
  for (;;)
  {
    if (_coders[ci].NumStreams != 1)
      break;
    if (!IsFilter_Vector[ci])
      break;
    
    UInt32 st = _bi.Coder_to_Stream[ci];
    if (_bi.IsStream_in_PackStreams(st))
      break;
    int bond = _bi.FindBond_for_PackStream(st);
    if (bond < 0)
      throw 20150213;
    ci = _bi.Bonds[bond].UnpackIndex;
  }
  
  MainCoderIndex = ci;
}

HRESULT CMixerMT::Init(ISequentialInStream * const *inStreams, ISequentialOutStream * const *outStreams)
{
  unsigned i;
  
  for (i = 0; i < _coders.Size(); i++)
  {
    CCoderMT &coderInfo = _coders[i];
    const CCoderStreamsInfo &csi = _bi.Coders[i];
    
    UInt32 j;

    unsigned numInStreams = EncodeMode ? 1 : csi.NumStreams;
    unsigned numOutStreams = EncodeMode ? csi.NumStreams : 1;

    coderInfo.InStreams.Clear();
    for (j = 0; j < numInStreams; j++)
      coderInfo.InStreams.AddNew();
    
    coderInfo.OutStreams.Clear();
    for (j = 0; j < numOutStreams; j++)
      coderInfo.OutStreams.AddNew();
  }

  for (i = 0; i < _bi.Bonds.Size(); i++)
  {
    const CBond &bond = _bi.Bonds[i];
   
    UInt32 inCoderIndex, inCoderStreamIndex;
    UInt32 outCoderIndex, outCoderStreamIndex;
    
    {
      UInt32 coderIndex, coderStreamIndex;
      _bi.GetCoder_for_Stream(bond.PackIndex, coderIndex, coderStreamIndex);

      inCoderIndex = EncodeMode ? bond.UnpackIndex : coderIndex;
      outCoderIndex = EncodeMode ? coderIndex : bond.UnpackIndex;

      inCoderStreamIndex = EncodeMode ? 0 : coderStreamIndex;
      outCoderStreamIndex = EncodeMode ? coderStreamIndex : 0;
    }

    _streamBinders[i].CreateStreams(
        &_coders[inCoderIndex].InStreams[inCoderStreamIndex],
        &_coders[outCoderIndex].OutStreams[outCoderStreamIndex]);

    CMyComPtr<ICompressSetBufSize> inSetSize, outSetSize;
    _coders[inCoderIndex].QueryInterface(IID_ICompressSetBufSize, (void **)&inSetSize);
    _coders[outCoderIndex].QueryInterface(IID_ICompressSetBufSize, (void **)&outSetSize);
    if (inSetSize && outSetSize)
    {
      const UInt32 kBufSize = 1 << 19;
      inSetSize->SetInBufSize(inCoderStreamIndex, kBufSize);
      outSetSize->SetOutBufSize(outCoderStreamIndex, kBufSize);
    }
  }

  {
    CCoderMT &cod = _coders[_bi.UnpackCoder];
    if (EncodeMode)
      cod.InStreams[0] = inStreams[0];
    else
      cod.OutStreams[0] = outStreams[0];
  }

  for (i = 0; i < _bi.PackStreams.Size(); i++)
  {
    UInt32 coderIndex, coderStreamIndex;
    _bi.GetCoder_for_Stream(_bi.PackStreams[i], coderIndex, coderStreamIndex);
    CCoderMT &cod = _coders[coderIndex];
    if (EncodeMode)
      cod.OutStreams[coderStreamIndex] = outStreams[i];
    else
      cod.InStreams[coderStreamIndex] = inStreams[i];
  }
  
  return S_OK;
}

HRESULT CMixerMT::ReturnIfError(HRESULT code)
{
  FOR_VECTOR (i, _coders)
    if (_coders[i].Result == code)
      return code;
  return S_OK;
}

HRESULT CMixerMT::Code(
    ISequentialInStream * const *inStreams,
    ISequentialOutStream * const *outStreams,
    ICompressProgressInfo *progress)
{
  Init(inStreams, outStreams);

  unsigned i;
  for (i = 0; i < _coders.Size(); i++)
    if (i != MainCoderIndex)
    {
      RINOK(_coders[i].Create());
    }

  for (i = 0; i < _coders.Size(); i++)
    if (i != MainCoderIndex)
      _coders[i].Start();

  _coders[MainCoderIndex].Code(progress);

  for (i = 0; i < _coders.Size(); i++)
    if (i != MainCoderIndex)
      _coders[i].WaitExecuteFinish();

  RINOK(ReturnIfError(E_ABORT));
  RINOK(ReturnIfError(E_OUTOFMEMORY));

  for (i = 0; i < _coders.Size(); i++)
  {
    HRESULT result = _coders[i].Result;
    if (result != S_OK
        && result != k_My_HRESULT_WritingWasCut
        && result != S_FALSE
        && result != E_FAIL)
      return result;
  }

  RINOK(ReturnIfError(S_FALSE));

  for (i = 0; i < _coders.Size(); i++)
  {
    HRESULT result = _coders[i].Result;
    if (result != S_OK && result != k_My_HRESULT_WritingWasCut)
      return result;
  }

  return S_OK;
}

UInt64 CMixerMT::GetBondStreamSize(unsigned bondIndex) const
{
  return _streamBinders[bondIndex].ProcessedSize;
}

}
