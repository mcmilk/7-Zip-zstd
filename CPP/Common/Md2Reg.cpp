// Md2Reg.cpp /TR 2018-11-02

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/md2.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// MD2
Z7_CLASS_IMP_COM_1(
  CMD2Hasher
  , IHasher
)
  MD2_CTX _ctx;

public:
  CMD2Hasher() { MD2_Init(&_ctx); }
};

Z7_COM7F_IMF2(void, CMD2Hasher::Init())
{
  MD2_Init(&_ctx);
}

Z7_COM7F_IMF2(void, CMD2Hasher::Update(const void *data, UInt32 size))
{
  MD2_Update(&_ctx, (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CMD2Hasher::Final(Byte *digest))
{
  MD2_Final(digest, &_ctx);
}
REGISTER_HASHER(CMD2Hasher, 0x205, "MD2", MD2_DIGEST_LENGTH)
