// CoderMixer2ST.h

#ifndef __CODER_MIXER2_ST_H
#define __CODER_MIXER2_ST_H

#include "CoderMixer2.h"
#include "../../../Common/MyCom.h"
#include "../../ICoder.h"

namespace NCoderMixer2 {

//  SetBindInfo()
//  for each coder
//  {
//    AddCoder[2]()
//  }
// 
//  for each file
//  {
//    ReInit()
//    for each coder
//    {
//      SetCoderInfo  
//    }
//    SetProgressIndex(UInt32 coderIndex);
//    Code
//  }

struct CSTCoderInfo: public CCoderInfo
{
  bool IsMain;
  CSTCoderInfo(UInt32 numInStreams, UInt32 numOutStreams, bool isMain):
    CCoderInfo(numInStreams, numOutStreams),IsMain(isMain) {}
};

class CCoderMixer2ST:
  public ICompressCoder2,
  public CCoderMixer2,
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP

  HRESULT GetInStream(
    ISequentialInStream **inStreams, const UInt64 **inSizes, 
    UInt32 streamIndex, ISequentialInStream **inStreamRes);
  HRESULT GetOutStream(
    ISequentialOutStream **outStreams, const UInt64 **outSizes, 
    UInt32 streamIndex, ISequentialOutStream **outStreamRes);
public:
  STDMETHOD(Code)(ISequentialInStream **inStreams,
      const UInt64 **inSizes, 
      UInt32 numInStreams,
      ISequentialOutStream **outStreams, 
      const UInt64 **outSizes,
      UInt32 numOutStreams,
      ICompressProgressInfo *progress);

  CCoderMixer2ST();
  ~CCoderMixer2ST();
  void AddCoderCommon(bool isMain);
  void AddCoder(ICompressCoder *coder, bool isMain);
  void AddCoder2(ICompressCoder2 *coder, bool isMain);

  void ReInit();
  void SetCoderInfo(UInt32 coderIndex, const UInt64 **inSizes, const UInt64 **outSizes)
  {  
    {  _coders[coderIndex].SetCoderInfo(inSizes, outSizes); }
  }

  void SetProgressCoderIndex(UInt32 /*coderIndex*/)
  {  
    // _progressCoderIndex = coderIndex; 
  }

  // UInt64 GetWriteProcessedSize(UInt32 binderIndex) const;

private:
  CBindInfo _bindInfo;
  CObjectVector<CSTCoderInfo> _coders;
  int _mainCoderIndex;
public:
  HRESULT SetBindInfo(const CBindInfo &bindInfo);

};

}
#endif

