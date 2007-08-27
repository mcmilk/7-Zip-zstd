// CoderMixerMT.h

#ifndef __CODER_MIXER_MT_H
#define __CODER_MIXER_MT_H

#include "../../../Common/MyVector.h"
#include "../../../Common/MyCom.h"
#include "../../ICoder.h"
#include "../../Common/StreamBinder.h"
#include "../../Common/VirtThread.h"
#include "CoderMixer.h"

namespace NCoderMixer {

struct CCoder: public CCoderInfo, public CVirtThread
{
  HRESULT Result;

  virtual void Execute();
  void Code(ICompressProgressInfo *progress);
};

/*
  for each coder
    AddCoder()
  SetProgressIndex(UInt32 coderIndex);
 
  for each file
  {
    ReInit()
    for each coder
      SetCoderInfo  
    Code
  }
*/


class CCoderMixerMT:
  public ICompressCoder,
  public CMyUnknownImp
{
  CObjectVector<CStreamBinder> _streamBinders;
  int _progressCoderIndex;

  HRESULT ReturnIfError(HRESULT code);
public:
  CObjectVector<CCoder> _coders;
  MY_UNKNOWN_IMP

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  void AddCoder(ICompressCoder *coder);
  void SetProgressCoderIndex(int coderIndex) {  _progressCoderIndex = coderIndex; }

  void ReInit();
  void SetCoderInfo(UInt32 coderIndex, const UInt64 *inSize, const UInt64 *outSize)
    {  _coders[coderIndex].SetCoderInfo(inSize, outSize); }

  /*
  UInt64 GetWriteProcessedSize(UInt32 binderIndex) const
    {  return _streamBinders[binderIndex].ProcessedSize; }
  */
};

}
#endif
