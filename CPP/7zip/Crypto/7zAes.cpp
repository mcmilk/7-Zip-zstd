// 7zAes.cpp

#include "StdAfx.h"

#include "../../../C/Sha256.h"

#include "Windows/Synchronization.h"

#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "7zAes.h"
#include "MyAes.h"

#ifndef EXTRACT_ONLY
#include "RandGen.h"
#endif

using namespace NWindows;

namespace NCrypto {
namespace NSevenZ {

bool CKeyInfo::IsEqualTo(const CKeyInfo &a) const
{
  if (SaltSize != a.SaltSize || NumCyclesPower != a.NumCyclesPower)
    return false;
  for (UInt32 i = 0; i < SaltSize; i++)
    if (Salt[i] != a.Salt[i])
      return false;
  return (Password == a.Password);
}

void CKeyInfo::CalculateDigest()
{
  if (NumCyclesPower == 0x3F)
  {
    UInt32 pos;
    for (pos = 0; pos < SaltSize; pos++)
      Key[pos] = Salt[pos];
    for (UInt32 i = 0; i < Password.GetCapacity() && pos < kKeySize; i++)
      Key[pos++] = Password[i];
    for (; pos < kKeySize; pos++)
      Key[pos] = 0;
  }
  else
  {
    CSha256 sha;
    Sha256_Init(&sha);
    const UInt64 numRounds = (UInt64)1 << NumCyclesPower;
    Byte temp[8] = { 0,0,0,0,0,0,0,0 };
    for (UInt64 round = 0; round < numRounds; round++)
    {
      Sha256_Update(&sha, Salt, (size_t)SaltSize);
      Sha256_Update(&sha, Password, Password.GetCapacity());
      Sha256_Update(&sha, temp, 8);
      for (int i = 0; i < 8; i++)
        if (++(temp[i]) != 0)
          break;
    }
    Sha256_Final(&sha, Key);
  }
}

bool CKeyInfoCache::Find(CKeyInfo &key)
{
  for (int i = 0; i < Keys.Size(); i++)
  {
    const CKeyInfo &cached = Keys[i];
    if (key.IsEqualTo(cached))
    {
      for (int j = 0; j < kKeySize; j++)
        key.Key[j] = cached.Key[j];
      if (i != 0)
      {
        Keys.Insert(0, cached);
        Keys.Delete(i+1);
      }
      return true;
    }
  }
  return false;
}

void CKeyInfoCache::Add(CKeyInfo &key)
{
  if (Find(key))
    return;
  if (Keys.Size() >= Size)
    Keys.DeleteBack();
  Keys.Insert(0, key);
}

static CKeyInfoCache g_GlobalKeyCache(32);
static NSynchronization::CCriticalSection g_GlobalKeyCacheCriticalSection;

CBase::CBase():
  _cachedKeys(16),
  _ivSize(0)
{
  for (int i = 0; i < sizeof(_iv); i++)
    _iv[i] = 0;
}

void CBase::CalculateDigest()
{
  NSynchronization::CCriticalSectionLock lock(g_GlobalKeyCacheCriticalSection);
  if (_cachedKeys.Find(_key))
    g_GlobalKeyCache.Add(_key);
  else
  {
    if (!g_GlobalKeyCache.Find(_key))
    {
      _key.CalculateDigest();
      g_GlobalKeyCache.Add(_key);
    }
    _cachedKeys.Add(_key);
  }
}

#ifndef EXTRACT_ONLY

/*
STDMETHODIMP CEncoder::ResetSalt()
{
  _key.SaltSize = 4;
  g_RandomGenerator.Generate(_key.Salt, _key.SaltSize);
  return S_OK;
}
*/

STDMETHODIMP CEncoder::ResetInitVector()
{
  _ivSize = 8;
  g_RandomGenerator.Generate(_iv, (unsigned)_ivSize);
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{
   // _key.Init();
   for (UInt32 i = _ivSize; i < sizeof(_iv); i++)
    _iv[i] = 0;

  UInt32 ivSize = _ivSize;
  
  // _key.NumCyclesPower = 0x3F;
  _key.NumCyclesPower = 19;

  Byte firstByte = (Byte)(_key.NumCyclesPower |
    (((_key.SaltSize == 0) ? 0 : 1) << 7) |
    (((ivSize == 0) ? 0 : 1) << 6));
  RINOK(outStream->Write(&firstByte, 1, NULL));
  if (_key.SaltSize == 0 && ivSize == 0)
    return S_OK;
  Byte saltSizeSpec = (Byte)((_key.SaltSize == 0) ? 0 : (_key.SaltSize - 1));
  Byte ivSizeSpec = (Byte)((ivSize == 0) ? 0 : (ivSize - 1));
  Byte secondByte = (Byte)(((saltSizeSpec) << 4) | ivSizeSpec);
  RINOK(outStream->Write(&secondByte, 1, NULL));
  if (_key.SaltSize > 0)
  {
    RINOK(WriteStream(outStream, _key.Salt, _key.SaltSize));
  }
  if (ivSize > 0)
  {
    RINOK(WriteStream(outStream, _iv, ivSize));
  }
  return S_OK;
}

HRESULT CEncoder::CreateFilter()
{
  _aesFilter = new CAesCbcEncoder;
  return S_OK;
}

#endif

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size)
{
  _key.Init();
  UInt32 i;
  for (i = 0; i < sizeof(_iv); i++)
    _iv[i] = 0;
  if (size == 0)
    return S_OK;
  UInt32 pos = 0;
  Byte firstByte = data[pos++];

  _key.NumCyclesPower = firstByte & 0x3F;
  if ((firstByte & 0xC0) == 0)
    return S_OK;
  _key.SaltSize = (firstByte >> 7) & 1;
  UInt32 ivSize = (firstByte >> 6) & 1;

  if (pos >= size)
    return E_INVALIDARG;
  Byte secondByte = data[pos++];
  
  _key.SaltSize += (secondByte >> 4);
  ivSize += (secondByte & 0x0F);
  
  if (pos + _key.SaltSize + ivSize > size)
    return E_INVALIDARG;
  for (i = 0; i < _key.SaltSize; i++)
    _key.Salt[i] = data[pos++];
  for (i = 0; i < ivSize; i++)
    _iv[i] = data[pos++];
  return (_key.NumCyclesPower <= 24) ? S_OK :  E_NOTIMPL;
}

STDMETHODIMP CBaseCoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  _key.Password.SetCapacity((size_t)size);
  memcpy(_key.Password, data, (size_t)size);
  return S_OK;
}

STDMETHODIMP CBaseCoder::Init()
{
  CalculateDigest();
  if (_aesFilter == 0)
  {
    RINOK(CreateFilter());
  }
  CMyComPtr<ICryptoProperties> cp;
  RINOK(_aesFilter.QueryInterface(IID_ICryptoProperties, &cp));
  RINOK(cp->SetKey(_key.Key, sizeof(_key.Key)));
  RINOK(cp->SetInitVector(_iv, sizeof(_iv)));
  return S_OK;
}

STDMETHODIMP_(UInt32) CBaseCoder::Filter(Byte *data, UInt32 size)
{
  return _aesFilter->Filter(data, size);
}

HRESULT CDecoder::CreateFilter()
{
  _aesFilter = new CAesCbcDecoder;
  return S_OK;
}

}}
