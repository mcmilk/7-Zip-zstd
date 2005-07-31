// sha1.h
// This file is based on public domain 
// Steve Reid and Wei Dai's code from Crypto++

#ifndef __RAR_SHA1
#define __RAR_SHA1

#include <stddef.h>
#include "../../../Common/Types.h"

#define HW 5

struct CSHA1
{
  UInt32 m_State[5];
  UInt64 m_Count;
  unsigned char _buffer[64];

  void Transform(const UInt32 data[16]);
  void WriteByteBlock();
  
  void Init();
  void Update(const Byte *data, size_t size);
  void Final(Byte *digest);
};

#endif
