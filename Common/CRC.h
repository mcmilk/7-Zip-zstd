// Common/CRC.h

#ifndef __COMMON_CRC_H
#define __COMMON_CRC_H

#include "Types.h"

class CCRC
{
  UInt32 _value;
public:
	static UInt32 Table[256];
	static void InitTable();

  CCRC():  _value(0xFFFFFFFF){};
  void Init() { _value = 0xFFFFFFFF; }
  void UpdateByte(Byte v);
  void UpdateUInt16(UInt16 v);
  void UpdateUInt32(UInt32 v);
  void UpdateUInt64(UInt64 v);
  void Update(const void *data, UInt32 size);
  UInt32 GetDigest() const { return _value ^ 0xFFFFFFFF; } 
  static UInt32 CalculateDigest(const void *data, UInt32 size)
  {
    CCRC crc;
    crc.Update(data, size);
    return crc.GetDigest();
  }
  static bool VerifyDigest(UInt32 digest, const void *data, UInt32 size)
  {
    return (CalculateDigest(data, size) == digest);
  }
};

#endif
