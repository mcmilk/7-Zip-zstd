// Crypto/HASH/SHA256/SHA256.cpp

#include "StdAfx.h"

#include "MySHA256.h"
#include "Windows/Defs.h"

namespace NCrypto {
namespace NSHA256 {


STDMETHODIMP CHash::Init()
{
  _SHA.Init();
  return S_OK;
}

STDMETHODIMP CHash::Update(const void *data, UINT32 size)
{
  _SHA.Update((const BYTE *)data, size);
  return S_OK;
}

STDMETHODIMP CHash::GetDigest(BYTE *digestData)
{
  _SHA.Final(digestData);
  return S_OK;
}

STDMETHODIMP CHash::GetDigestSize(UINT32 *size)
{
  *size = SHA256::DIGESTSIZE;
  return S_OK;
}

}}
