// Common/CRC.h

#pragma once

#ifndef __COMMON_CRC_H
#define __COMMON_CRC_H

#include "Common/Types.h"

class CCRC
{
  UINT32 m_Value;
public:
	static UINT32 m_Table[256];
  CCRC():  m_Value(0xFFFFFFFF){};
  void Init() { m_Value = 0xFFFFFFFF; }
  void Update(const void *aData, UINT32 aSize);
  UINT32 GetDigest() const { return m_Value ^ 0xFFFFFFFF; } 
  static UINT32 CalculateDigest(const void *aData, UINT32 aSize)
  {
    CCRC aCRC;
    aCRC.Update(aData, aSize);
    return aCRC.GetDigest();
  }
  static bool VerifyDigest(UINT32 aDigest, const void *aData, UINT32 aSize)
  {
    return (CalculateDigest(aData, aSize) == aDigest);
  }
};

#endif
