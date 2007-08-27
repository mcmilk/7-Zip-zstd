// CoderMixerMT.cpp

#include "StdAfx.h"

#include "CoderMixerMT.h"

namespace NCoderMixer {

void CCoder::Execute() { Code(NULL); }

void CCoder::Code(ICompressProgressInfo *progress)
{
  Result = Coder->Code(InStream, OutStream, 
      InSizeAssigned ? &InSizeValue : NULL, 
      OutSizeAssigned ? &OutSizeValue : NULL, 
      progress);
  InStream.Release();
  OutStream.Release();
}

void CCoderMixerMT::AddCoder(ICompressCoder *coder)
{
  _coders.Add(CCoder());
  _coders.Back().Coder = coder;
}

void CCoderMixerMT::ReInit()
{
  for(int i = 0; i < _coders.Size(); i++)
    _coders[i].ReInit();
}

HRESULT CCoderMixerMT::ReturnIfError(HRESULT code)
{
  for (int i = 0; i < _coders.Size(); i++)
    if (_coders[i].Result == code)
      return code;
  return S_OK;
}

STDMETHODIMP CCoderMixerMT::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UInt64 * /* inSize */, const UInt64 * /* outSize */,
    ICompressProgressInfo *progress)
{
  _coders.Front().InStream = inStream;
  int i;
  _coders.Back().OutStream = outStream;

  for (i = 0; i < _coders.Size(); i++)
    if (i != _progressCoderIndex)
    {
      RINOK(_coders[i].Create());
    }

  _streamBinders.Clear();
  for (i = 0; i + 1 < _coders.Size(); i++)
  {
    _streamBinders.Add(CStreamBinder());
    CStreamBinder &sb = _streamBinders[i];
    RINOK(sb.CreateEvents());
    sb.CreateStreams(&_coders[i + 1].InStream, &_coders[i].OutStream);
  }

  for(i = 0; i < _streamBinders.Size(); i++)
    _streamBinders[i].ReInit();

  for (i = 0; i < _coders.Size(); i++)
    if (i != _progressCoderIndex)
      _coders[i].Start();

  _coders[_progressCoderIndex].Code(progress);

  for (i = 0; i < _coders.Size(); i++)
    if (i != _progressCoderIndex)
      _coders[i].WaitFinish();

  RINOK(ReturnIfError(E_ABORT));
  RINOK(ReturnIfError(E_OUTOFMEMORY));
  RINOK(ReturnIfError(S_FALSE));

  for (i = 0; i < _coders.Size(); i++)
  {
    HRESULT result = _coders[i].Result;
    if (result != S_OK && result != E_FAIL)
      return result;
  }
  for (i = 0; i < _coders.Size(); i++)
  {
    HRESULT result = _coders[i].Result;
    if (result != S_OK)
      return result;
  }
  return S_OK;
}

}  
