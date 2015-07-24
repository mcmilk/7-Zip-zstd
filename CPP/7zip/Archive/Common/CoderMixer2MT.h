// CoderMixer2MT.h

#ifndef __CODER_MIXER2_MT_H
#define __CODER_MIXER2_MT_H

#include "../../../Common/MyCom.h"

#include "../../Common/StreamBinder.h"
#include "../../Common/VirtThread.h"

#include "CoderMixer2.h"

namespace NCoderMixer2 {

class CCoderMT: public CCoder, public CVirtThread
{
  CLASS_NO_COPY(CCoderMT)
  CRecordVector<ISequentialInStream*> InStreamPointers;
  CRecordVector<ISequentialOutStream*> OutStreamPointers;

private:
  void Execute();
public:
  bool EncodeMode;
  HRESULT Result;
  CObjectVector< CMyComPtr<ISequentialInStream> > InStreams;
  CObjectVector< CMyComPtr<ISequentialOutStream> > OutStreams;

  CCoderMT(): EncodeMode(false) {}
  ~CCoderMT() { CVirtThread::WaitThreadFinish(); }
  
  void Code(ICompressProgressInfo *progress);
};



class CMixerMT:
  public IUnknown,
  public CMixer,
  public CMyUnknownImp
{
  CObjectVector<CStreamBinder> _streamBinders;

  HRESULT Init(ISequentialInStream * const *inStreams, ISequentialOutStream * const *outStreams);
  HRESULT ReturnIfError(HRESULT code);

public:
  CObjectVector<CCoderMT> _coders;

  MY_UNKNOWN_IMP

  virtual HRESULT SetBindInfo(const CBindInfo &bindInfo);

  virtual void AddCoder(ICompressCoder *coder, ICompressCoder2 *coder2, bool isFilter);

  virtual CCoder &GetCoder(unsigned index);

  virtual void SelectMainCoder(bool useFirst);

  virtual void ReInit();
  
  virtual void SetCoderInfo(unsigned coderIndex, const UInt64 *unpackSize, const UInt64 * const *packSizes)
    { _coders[coderIndex].SetCoderInfo(unpackSize, packSizes); }
  
  virtual HRESULT Code(
      ISequentialInStream * const *inStreams,
      ISequentialOutStream * const *outStreams,
      ICompressProgressInfo *progress);

  virtual UInt64 GetBondStreamSize(unsigned bondIndex) const;

  CMixerMT(bool encodeMode): CMixer(encodeMode) {}
};

}

#endif
