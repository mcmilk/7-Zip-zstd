// MethodInfo.h

#pragma once

#ifndef __7Z_METHODINFO_H
#define __7Z_METHODINFO_H

#include "Common/String.h"
#include "Common/Buffer.h"

namespace NArchive {
namespace N7z {

const int kMethodIDSize = 16;
  
struct CMethodID
{
  BYTE ID[kMethodIDSize];
  UINT32 IDSize;
  AString ConvertToString() const;
  bool ConvertFromString(const AString &srcString);
};

inline bool operator==(const CMethodID &a1, const CMethodID &a2)
{
  if (a1.IDSize != a2.IDSize)
    return false;
  for (UINT32 i = 0; i < a1.IDSize; i++)
    if (a1.ID[i] != a2.ID[i])
      return false;
  return true;
}

inline bool operator!=(const CMethodID &a1, const CMethodID &a2)
{
  return !(a1 == a2);
}

}}

#endif
