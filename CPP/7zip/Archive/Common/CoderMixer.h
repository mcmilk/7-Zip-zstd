// CoderMixer.h

#ifndef __CODER_MIXER_H
#define __CODER_MIXER_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"

namespace NCoderMixer {

struct CCoderInfo
{
  CMyComPtr<ICompressCoder> Coder;
  CMyComPtr<ISequentialInStream> InStream;
  CMyComPtr<ISequentialOutStream> OutStream;
  CMyComPtr<ICompressProgressInfo> Progress;

  UInt64 InSizeValue;
  UInt64 OutSizeValue;
  bool InSizeAssigned;
  bool OutSizeAssigned;

  void ReInit()
  {
    InSizeAssigned = OutSizeAssigned = false;
  }

  void SetCoderInfo(const UInt64 *inSize, const UInt64 *outSize);
};

}
#endif
