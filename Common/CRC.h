// Common/CRC.h

// #pragma once

#ifndef __COMMON_CRC_H
#define __COMMON_CRC_H

#include "Types.h"

class CCRC
{
  UINT32 _value;
public:
	static UINT32 Table[256];
  CCRC():  _value(0xFFFFFFFF){};
  void Init() { _value = 0xFFFFFFFF; }
  void Update(const void *data, UINT32 size);
  UINT32 GetDigest() const { return _value ^ 0xFFFFFFFF; } 
  static UINT32 CalculateDigest(const void *data, UINT32 size)
  {
    CCRC crc;
    crc.Update(data, size);
    return crc.GetDigest();
  }
  static bool VerifyDigest(UINT32 digest, const void *data, UINT32 size)
  {
    return (CalculateDigest(data, size) == digest);
  }
};

#endif
