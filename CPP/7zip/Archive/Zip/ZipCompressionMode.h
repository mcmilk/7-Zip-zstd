// CompressionMode.h

#ifndef __ZIP_COMPRESSION_MODE_H
#define __ZIP_COMPRESSION_MODE_H

#include "Common/MyString.h"

namespace NArchive {
namespace NZip {

struct CCompressionMethodMode
{
  CRecordVector<Byte> MethodSequence;
  UString MatchFinder;
  UInt32 Algo;
  UInt32 NumPasses;
  UInt32 NumFastBytes;
  bool NumMatchFinderCyclesDefined;
  UInt32 NumMatchFinderCycles;
  UInt32 DicSize;
  UInt32 MemSize;
  UInt32 Order;

  #ifndef _7ZIP_ST
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
