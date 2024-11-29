// CksumReg.cpp

#include "StdAfx.h"

#include "../../C/CpuArch.h"

#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"
#include "../7zip/Compress/BZip2Crc.h"

Z7_CLASS_IMP_COM_1(
  CCksumHasher
  , IHasher
)
  CBZip2Crc _crc;
  UInt64 _size;
public:
  // Byte _mtDummy[1 << 7];
  CCksumHasher()
  {
    _crc.Init(0);
    _size = 0;
  }
};

Z7_COM7F_IMF2(void, CCksumHasher::Init())
{
  _crc.Init(0);
  _size = 0;
}

Z7_COM7F_IMF2(void, CCksumHasher::Update(const void *data, UInt32 size))
{
  _size += size;
  CBZip2Crc crc = _crc;
  for (UInt32 i = 0; i < size; i++)
    crc.UpdateByte(((const Byte *)data)[i]);
  _crc = crc;
}

Z7_COM7F_IMF2(void, CCksumHasher::Final(Byte *digest))
{
  UInt64 size = _size;
  CBZip2Crc crc = _crc;
  while (size)
  {
    crc.UpdateByte((Byte)size);
    size >>= 8;
  }
  const UInt32 val = crc.GetDigest();
  SetUi32(digest, val)
}

REGISTER_HASHER(CCksumHasher, 0x203, "CKSUM", 4)
