// BZip2CRC.h

#ifndef __BZIP2_CRC_H
#define __BZIP2_CRC_H

#include "Common/Types.h"

class CBZip2CRC
{
  UInt32 _value;
  static UInt32 Table[256];
public:
  static void InitTable();
  CBZip2CRC(): _value(0xFFFFFFFF) {};
  void Init() { _value = 0xFFFFFFFF; }
  void UpdateByte(Byte b) { _value = Table[(_value >> 24) ^ b] ^ (_value << 8); }
  void UpdateByte(unsigned int b) { _value = Table[(_value >> 24) ^ b] ^ (_value << 8); }
  UInt32 GetDigest() const { return _value ^ 0xFFFFFFFF; } 
};

class CBZip2CombinedCRC
{
  UInt32 _value;
public:
  CBZip2CombinedCRC():  _value(0){};
  void Init() { _value = 0; }
  void Update(UInt32 v) { _value = ((_value << 1) | (_value >> 31)) ^ v; }
  UInt32 GetDigest() const { return _value ; } 
};

#endif
