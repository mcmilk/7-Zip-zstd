// CoderMixer.cpp

#include "StdAfx.h"

#include "CoderMixer.h"

namespace NCoderMixer {

void CCoderInfo::SetCoderInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  InSizeAssigned = (inSize != 0);
  if (InSizeAssigned)
    InSizeValue = *inSize;
  OutSizeAssigned = (outSize != 0);
  if (OutSizeAssigned)
    OutSizeValue = *outSize;
}

}  
