// sha1.h
// This file is based on public domain 
// Steve Reid and Wei Dai's code from Crypto++

#ifndef __RAR_SHA1
#define __RAR_SHA1

#include <stddef.h>
#include "../../../Common/Types.h"

#define HW 5

// Sha1 implementation in RAR before version 3.60 has bug:
// it changes data bytes in some cases.
// So this class supports both versions: normal_SHA and rar3Mode

struct CSHA1
{
  UInt32 m_State[5];
  UInt64 m_Count;
  unsigned char _buffer[64];

  void Transform(UInt32 data[16], bool returnRes = false);
  void WriteByteBlock(bool returnRes = false);
  
public:
  void Init();
  void Update(Byte *data, size_t size, bool rar350Mode = false);
  void Final(Byte *digest);
};

#endif
