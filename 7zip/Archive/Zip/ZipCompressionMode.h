// CompressionMode.h

#ifndef __ZIP_COMPRESSIONMETHOD_H
#define __ZIP_COMPRESSIONMETHOD_H

#include "Common/Vector.h"
#include "Common/String.h"

namespace NArchive {
namespace NZip {

struct CCompressionMethodMode
{
  CRecordVector<Byte> MethodSequence;
  // bool MaximizeRatio;
  UInt32 NumPasses;
  UInt32 NumFastBytes;
  bool PasswordIsDefined;
  AString Password;
};

}}

#endif
