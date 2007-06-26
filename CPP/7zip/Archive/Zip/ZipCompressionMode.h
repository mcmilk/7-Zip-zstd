// CompressionMode.h

#ifndef __ZIP_COMPRESSIONMETHOD_H
#define __ZIP_COMPRESSIONMETHOD_H

#include "Common/MyString.h"

namespace NArchive {
namespace NZip {

struct CCompressionMethodMode
{
  CRecordVector<Byte> MethodSequence;
  // bool MaximizeRatio;
  UInt32 Algo;
  UInt32 NumPasses;
  UInt32 NumFastBytes;
  bool NumMatchFinderCyclesDefined;
  UInt32 NumMatchFinderCycles;
  UInt32 DicSize;
  #ifdef COMPRESS_MT
  UInt32 NumThreads;
  #endif
  bool PasswordIsDefined;
  AString Password;
  bool IsAesMode;
  Byte AesKeyMode;
  
  CCompressionMethodMode(): 
      NumMatchFinderCyclesDefined(false), 
      PasswordIsDefined(false), 
      IsAesMode(false), 
      AesKeyMode(3) 
      {} 
};

}}

#endif
