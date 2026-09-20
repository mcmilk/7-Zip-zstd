// 7zAes.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../../Common/ComTry.h"

#ifndef Z7_ST
#include "../../Windows/Synchronization.h"
#endif

#include "../Common/StreamUtils.h"

#include "7zAes.h"
#include "MyAes.h"

#ifndef Z7_EXTRACT_ONLY
#include "RandGen.h"
#endif

namespace NCrypto {
namespace N7z {

static const unsigned k_NumCyclesPower_Supported_MAX = 24;

static CKeyInfoCache g_GlobalKeyCache(32);

#ifndef Z7_ST
  static NWindows::NSynchronization::CCriticalSection g_GlobalKeyCacheCriticalSection;
  #define MT_LOCK NWindows::NSynchronization::CCriticalSectionLock lock(g_GlobalKeyCacheCriticalSection);
#else
  #define MT_LOCK
#endif

CBase::CBase():
  _cachedKeys(16),
  _ivSize(0)
{
  for (unsigned i = 0; i < sizeof(_iv); i++)
    _iv[i] = 0;
}

void CBase::PrepareKey()
{
  MT_LOCK
  
  bool finded = false;
  if (!_cachedKeys.GetKey(_key))
  {
    finded = g_GlobalKeyCache.GetKey(_key);
    if (!finded)
      _key.CalcKey();
    _cachedKeys.Add(_key);
  }
  if (!finded)
    g_GlobalKeyCache.FindAndAdd(_key);
}

#ifndef Z7_EXTRACT_ONLY

Z7_COM7F_IMF(CEncoder::ResetInitVector())
{
  for (unsigned i = 0; i < sizeof(_iv); i++)
    _iv[i] = 0;
  _ivSize = 16;
  MY_RAND_GEN(_iv, _ivSize);
  return S_OK;
}

Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  Byte props[2 + sizeof(_key.Salt) + sizeof(_iv)];
  unsigned propsSize = 1;

  props[0] = (Byte)(_key.NumCyclesPower
      | (_key.SaltSize == 0 ? 0 : (1 << 7))
      | (_ivSize       == 0 ? 0 : (1 << 6)));

  if (_key.SaltSize != 0 || _ivSize != 0)
  {
    props[1] = (Byte)(
        ((_key.SaltSize == 0 ? 0 : _key.SaltSize - 1) << 4)
        | (_ivSize      == 0 ? 0 : _ivSize - 1));
    memcpy(props + 2, _key.Salt, _key.SaltSize);
    propsSize = 2 + _key.SaltSize;
    memcpy(props + propsSize, _iv, _ivSize);
    propsSize += _ivSize;
  }

  return WriteStream(outStream, props, propsSize);
}

CEncoder::CEncoder()
{
  _key.NumCyclesPower = 19;
  _aesFilter = new CAesCbcEncoder(kKeySize);
}

#endif

CDecoder::CDecoder()
{
  _aesFilter = new CAesCbcDecoder(kKeySize);
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  _key.ClearProps();
 
  _ivSize = 0;
  unsigned i;
  for (i = 0; i < sizeof(_iv); i++)
    _iv[i] = 0;
  
  if (size == 0)
    return S_OK;
  
  const unsigned b0 = data[0];
  _key.NumCyclesPower = b0 & 0x3F;
  if ((b0 & 0xC0) == 0)
    return size == 1 ? S_OK : E_INVALIDARG;
  if (size <= 1)
    return E_INVALIDARG;

  const unsigned b1 = data[1];
  const unsigned saltSize = ((b0 >> 7) & 1) + (b1 >> 4);
  const unsigned ivSize   = ((b0 >> 6) & 1) + (b1 & 0x0F);
  
  if (size != 2 + saltSize + ivSize)
    return E_INVALIDARG;
  _key.SaltSize = saltSize;
  data += 2;
  for (i = 0; i < saltSize; i++)
    _key.Salt[i] = *data++;
  for (i = 0; i < ivSize; i++)
    _iv[i] = *data++;
  return (_key.NumCyclesPower <= k_NumCyclesPower_Supported_MAX
      || _key.NumCyclesPower == 0x3F) ? S_OK : E_NOTIMPL;
}


Z7_COM7F_IMF(CBaseCoder::CryptoSetPassword(const Byte *data, UInt32 size))
{
  COM_TRY_BEGIN
  
  _key.Password.Wipe();
  _key.Password.CopyFrom(data, (size_t)size);
  return S_OK;
  
  COM_TRY_END
}

Z7_COM7F_IMF(CBaseCoder::Init())
{
  COM_TRY_BEGIN
  
  PrepareKey();
  CMyComPtr<ICryptoProperties> cp;
  RINOK(_aesFilter.QueryInterface(IID_ICryptoProperties, &cp))
  if (!cp)
    return E_FAIL;
  RINOK(cp->SetKey(_key.Key, kKeySize))
  RINOK(cp->SetInitVector(_iv, sizeof(_iv)))
  return _aesFilter->Init();
  
  COM_TRY_END
}

Z7_COM7F_IMF2(UInt32, CBaseCoder::Filter(Byte *data, UInt32 size))
{
  return _aesFilter->Filter(data, size);
}

}}
