// 7z_AES.cpp

#include "StdAfx.h"

#include "Windows/Defs.h"
#include "Windows/Synchronization.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/StreamUtils.h"

#include "7zAES.h"
// #include "../../Hash/Common/CryptoHashInterface.h"

#ifdef CRYPTO_AES
#include "../AES/MyAES.h"
#endif

#include "SHA256.h"

using namespace NWindows;

#ifndef CRYPTO_AES
extern HINSTANCE g_hInstance;
#endif

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
    NCrypto::NSHA256::SHA256 sha;
    const UInt64 numRounds = UInt64(1) << (NumCyclesPower);
    Byte temp[8] = { 0,0,0,0,0,0,0,0 };
    for (UInt64 round = 0; round < numRounds; round++)
    {
      sha.Update(Salt, SaltSize);
      sha.Update(Password, Password.GetCapacity());
      sha.Update(temp, 8);
      for (int i = 0; i < 8; i++)
        if (++(temp[i]) != 0)
          break;
    }
    sha.Final(Key);
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
  _cachedKeys(16)
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


/*
static void GetRandomData(Byte *data)
{
  // probably we don't need truly random.
  // it's enough to prevent dictionary attack;
  // but it gives some info about time when compressing 
  // was made. 
  UInt64 tempValue;
  SYSTEMTIME systemTime;
  FILETIME fileTime;
  ::GetSystemTime(&systemTime);
  ::SystemTimeToFileTime(&systemTime, &fileTime);
  tempValue = *(const UInt64 *)&fileTime;
  LARGE_INTEGER counter;
  ::QueryPerformanceCounter(&counter);
  tempValue += *(const UInt64 *)&counter;
  tempValue += (UInt64)(GetTickCount()) << 32;
  *(UInt64 *)data = tempValue;
}
*/

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{ 
  _key.Init();
  for (UInt32 i = 0; i < sizeof(_iv); i++)
    _iv[i] = 0;

  _key.SaltSize = 0;
  
  // _key.SaltSize = 8;
  // GetRandomData(_key.Salt);

  int ivSize = 0;
  
  // _key.NumCyclesPower = 0x3F;
  _key.NumCyclesPower = 18;

  Byte firstByte = _key.NumCyclesPower | 
    (((_key.SaltSize == 0) ? 0 : 1) << 7) |
    (((ivSize == 0) ? 0 : 1) << 6);
  RINOK(outStream->Write(&firstByte, 1, NULL));
  if (_key.SaltSize == 0 && ivSize == 0)
    return S_OK;
  Byte saltSizeSpec = (_key.SaltSize == 0) ? 0 : (_key.SaltSize - 1);
  Byte ivSizeSpec = (ivSize == 0) ? 0 : (ivSize - 1);
  Byte secondByte = ((saltSizeSpec) << 4) | ivSizeSpec;
  RINOK(outStream->Write(&secondByte, 1, NULL));
  if (_key.SaltSize > 0)
  {
    RINOK(WriteStream(outStream, _key.Salt, _key.SaltSize, NULL));
  }
  if (ivSize > 0)
  {
    RINOK(WriteStream(outStream, _iv, ivSize, NULL));
  }
  return S_OK;
}

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
  return S_OK;
}

STDMETHODIMP CBaseCoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  _key.Password.SetCapacity(size);
  memcpy(_key.Password, data, size);
  return S_OK;
}

/*
static Byte *WideToRaw(const wchar_t *src, Byte *dest, int destSize=0x10000000)
{
  for (int i = 0; i < destSize; i++, src++)
  {
    dest[i * 2] = (Byte)*src;
    dest[i * 2 + 1]= (Byte)(*src >> 8);
    if (*src == 0)
      break;
  }
  return(dest);
}
*/

#ifndef CRYPTO_AES
bool GetAESLibPath(TCHAR *path)
{
  TCHAR fullPath[MAX_PATH + 1];
  if (::GetModuleFileName(g_hInstance, fullPath, MAX_PATH) == 0)
    return false;
  LPTSTR fileNamePointer;
  DWORD needLength = ::GetFullPathName(fullPath, MAX_PATH + 1, 
      path, &fileNamePointer);
  if (needLength == 0 || needLength >= MAX_PATH)
    return false;
  lstrcpy(fileNamePointer, TEXT("AES.dll"));
  return true;
}
#endif

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

#ifndef CRYPTO_AES
HRESULT CBaseCoder::CreateFilterFromDLL(REFCLSID clsID)
{
  if (!_aesLibrary)
  {
    TCHAR filePath[MAX_PATH + 2];
    if (!GetAESLibPath(filePath))
      return ::GetLastError();
    return _aesLibrary.LoadAndCreateFilter(filePath, clsID, &_aesFilter);
  }
  return S_OK;
}
#endif

HRESULT CEncoder::CreateFilter()
{
  #ifdef CRYPTO_AES
  _aesFilter = new CAES256_CBC_Encoder;
  return S_OK;
  #else
  return CreateFilterFromDLL(CLSID_CCrypto_AES256_Encoder);
  #endif
}

HRESULT CDecoder::CreateFilter()
{
  #ifdef CRYPTO_AES
  _aesFilter = new CAES256_CBC_Decoder;
  return S_OK;
  #else
  return CreateFilterFromDLL(CLSID_CCrypto_AES256_Decoder);
  #endif
}

}}
