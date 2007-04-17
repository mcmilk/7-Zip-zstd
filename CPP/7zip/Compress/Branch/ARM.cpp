// ARM.cpp

#include "StdAfx.h"
#include "ARM.h"

extern "C" 
{ 
#include "../../../../C/Compress/Branch/BranchARM.h"
}

UInt32 CBC_ARM_Encoder::SubFilter(Byte *data, UInt32 size)
{
  return ::ARM_Convert(data, size, _bufferPos, 1);
}

UInt32 CBC_ARM_Decoder::SubFilter(Byte *data, UInt32 size)
{
  return ::ARM_Convert(data, size, _bufferPos, 0);
}
