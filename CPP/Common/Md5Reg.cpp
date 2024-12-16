// Md5Reg.cpp /TR 2018-11-02

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/md5.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// MD5
Z7_CLASS_IMP_COM_1(
  CMD5Hasher
  , IHasher
)
  MD5_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CMD5Hasher() { MD5_Init(&_ctx); }
};

Z7_COM7F_IMF2(void, CMD5Hasher::Init())
{
  MD5_Init(&_ctx);
}

Z7_COM7F_IMF2(void, CMD5Hasher::Update(const void *data, UInt32 size))
{
  MD5_Update(&_ctx, (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CMD5Hasher::Final(Byte *digest))
{
  MD5_Final(digest, &_ctx);
}
REGISTER_HASHER(CMD5Hasher, 0x207, "MD5", MD5_DIGEST_LENGTH)
