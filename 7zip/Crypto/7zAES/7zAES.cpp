// 7z_AES.h

#include "StdAfx.h"

#include "Windows/Defs.h"
#include "Windows/Synchronization.h"
#include "../../Common/StreamObjects.h"

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
  for (UINT32 i = 0; i < SaltSize; i++)
    if (Salt[i] != a.Salt[i])
      return false;
  return (Password == a.Password);
}

void CKeyInfo::CalculateDigest()
{
  if (NumCyclesPower == 0x3F)
  {
    UINT32 pos;
    for (pos = 0; pos < SaltSize; pos++)
      Key[pos] = Salt[pos];
    for (UINT32 i = 0; i < Password.GetCapacity() && pos < kKeySize; i++)
      Key[pos++] = Password[i];
    for (; pos < kKeySize; pos++)
      Key[pos] = 0;
  }
  else
  {
    /*
    CMyComPtr<ICryptoHash> sha;
    RINOK(sha.CoCreateInstance(CLSID_CCrypto_Hash_SHA256));
    RINOK(sha->Init());
    */
    
    NCrypto::NSHA256::SHA256 sha;
    const UINT64 numRounds = UINT64(1) << (NumCyclesPower);
    for (UINT64 round = 0; round < numRounds; round++)
    {
      /*
      RINOK(sha->Update(Salt, SaltSize));
      RINOK(sha->Update(Password, Password.GetCapacity()));
      // change it if big endian;
      RINOK(sha->Update(&round, sizeof(round)));
      */
      
      // sha.Update(Salt, sizeof(Salt));
      sha.Update(Salt, SaltSize);
      sha.Update(Password, Password.GetCapacity());
      // change it if big endian;
      sha.Update((const BYTE *)&round, sizeof(round));
    }
    // return sha->GetDigest(Key);
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
static void GetRandomData(BYTE *data)
{
  // probably we don't need truly random.
  // it's enough to prevent dictionary attack;
  // but it gives some info about time when compressing 
  // was made. 
  UINT64 tempValue;
  SYSTEMTIME systemTime;
  FILETIME fileTime;
  ::GetSystemTime(&systemTime);
  ::SystemTimeToFileTime(&systemTime, &fileTime);
  tempValue = *(const UINT64 *)&fileTime;
  LARGE_INTEGER counter;
  ::QueryPerformanceCounter(&counter);
  tempValue += *(const UINT64 *)&counter;
  tempValue += (UINT64)(GetTickCount()) << 32;
  *(UINT64 *)data = tempValue;
}
*/

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{ 
  _key.Init();
  for (UINT32 i = 0; i < sizeof(_iv); i++)
    _iv[i] = 0;

  _key.SaltSize = 0;
  
  // _key.SaltSize = 8;
  // GetRandomData(_key.Salt);

  int ivSize = 0;
  
  // _key.NumCyclesPower = 0x3F;
  _key.NumCyclesPower = 18;

  BYTE firstByte = _key.NumCyclesPower | 
    (((_key.SaltSize == 0) ? 0 : 1) << 7) |
    (((ivSize == 0) ? 0 : 1) << 6);
  RINOK(outStream->Write(&firstByte, sizeof(firstByte), NULL));
  if (_key.SaltSize == 0 && ivSize == 0)
    return S_OK;
  BYTE saltSizeSpec = (_key.SaltSize == 0) ? 0 : (_key.SaltSize - 1);
  BYTE ivSizeSpec = (ivSize == 0) ? 0 : (ivSize - 1);
  BYTE secondByte = ((saltSizeSpec) << 4) | ivSizeSpec;
  RINOK(outStream->Write(&secondByte, sizeof(secondByte), NULL));
  if (_key.SaltSize > 0)
  {
    RINOK(outStream->Write(_key.Salt, _key.SaltSize, NULL));
  }
  if (ivSize > 0)
  {
    RINOK(outStream->Write(_iv, ivSize, NULL));
  }
  return S_OK;
}

STDMETHODIMP CEncoder::SetDecoderProperties(ISequentialInStream *inStream)
{
  return S_OK;
}

STDMETHODIMP CDecoder::SetDecoderProperties(ISequentialInStream *inStream)
{
  _key.Init();
  for (int i = 0; i < sizeof(_iv); i++)
    _iv[i] = 0;
  UINT32 processedSize;
  BYTE firstByte;
  RINOK(inStream->Read(&firstByte, sizeof(firstByte), &processedSize));
  if (processedSize == 0)
    return S_OK;

  _key.NumCyclesPower = firstByte & 0x3F;
  if ((firstByte & 0xC0) == 0)
    return S_OK;
  _key.SaltSize = (firstByte >> 7) & 1;
  UINT32 ivSize = (firstByte >> 6) & 1;

  BYTE secondByte;
  RINOK(inStream->Read(&secondByte, sizeof(secondByte), &processedSize));
  if (processedSize == 0)
    return E_INVALIDARG;
  
  _key.SaltSize += (secondByte >> 4);
  ivSize += (secondByte & 0x0F);
  
  RINOK(inStream->Read(_key.Salt, 
      _key.SaltSize, &processedSize));
  if (processedSize != _key.SaltSize)
    return E_INVALIDARG;

  RINOK(inStream->Read(_iv, ivSize, &processedSize));
  if (processedSize != ivSize)
    return E_INVALIDARG;

  return S_OK;
}

STDMETHODIMP CEncoder::CryptoSetPassword(const BYTE *data, UINT32 size)
{
  _key.Password.SetCapacity(size);
  memcpy(_key.Password, data, size);
  return S_OK;
}

STDMETHODIMP CDecoder::CryptoSetPassword(const BYTE *data, UINT32 size)
{
  _key.Password.SetCapacity(size);
  memcpy(_key.Password, data, size);
  return S_OK;
}

/*
static BYTE *WideToRaw(const wchar_t *src, BYTE *dest, int destSize=0x10000000)
{
  for (int i = 0; i < destSize; i++, src++)
  {
    dest[i * 2] = (BYTE)*src;
    dest[i * 2 + 1]= (BYTE)(*src >> 8);
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

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,ICompressProgressInfo *progress)
{
  CalculateDigest();

  if (_aesEncoder == 0)
  {
    #ifdef CRYPTO_AES
    _aesEncoder = new CAES256_CBC_Encoder;
    #else
    if ((HMODULE)_aesEncoderLibrary == 0)
    {
      TCHAR filePath[MAX_PATH + 2];
      if (!GetAESLibPath(filePath))
        return ::GetLastError();
      RINOK(_aesEncoderLibrary.LoadAndCreateCoder2(filePath, 
        CLSID_CCrypto_AES256_Encoder, &_aesEncoder));
    }
    #endif
  }

  CSequentialInStreamImp *ivStreamSpec = new CSequentialInStreamImp;
  CMyComPtr<ISequentialInStream> ivStream(ivStreamSpec);
  ivStreamSpec->Init(_iv, sizeof(_iv));

  CSequentialInStreamImp *keyStreamSpec = new CSequentialInStreamImp;
  CMyComPtr<ISequentialInStream> keyStream(keyStreamSpec);
  keyStreamSpec->Init(_key.Key, sizeof(_key.Key));

  ISequentialInStream *inStreams[3] = { inStream, ivStream, keyStream };
  UINT64 ivSize = sizeof(_iv);
  UINT64 keySize = sizeof(_key.Key);
  const UINT64 *inSizes[3] = { inSize, &ivSize, &ivSize, };
  return _aesEncoder->Code(inStreams, inSizes, 3, 
      &outStream, &outSize, 1, progress);
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,ICompressProgressInfo *progress)
{
  CalculateDigest();

  if (_aesDecoder == 0)
  {
    #ifdef CRYPTO_AES
    _aesDecoder = new CAES256_CBC_Decoder;
    #else
    if ((HMODULE)_aesDecoderLibrary == 0)
    {
      TCHAR filePath[MAX_PATH + 2];
      if (!GetAESLibPath(filePath))
        return ::GetLastError();
      RINOK(_aesDecoderLibrary.LoadAndCreateCoder2(filePath, 
        CLSID_CCrypto_AES256_Decoder, &_aesDecoder));
    }
    #endif
  }

  CSequentialInStreamImp *ivStreamSpec = new CSequentialInStreamImp;
  CMyComPtr<ISequentialInStream> ivStream(ivStreamSpec);
  ivStreamSpec->Init(_iv, sizeof(_iv));

  CSequentialInStreamImp *keyStreamSpec = new CSequentialInStreamImp;
  CMyComPtr<ISequentialInStream> keyStream(keyStreamSpec);
  keyStreamSpec->Init(_key.Key, sizeof(_key.Key));

  ISequentialInStream *inStreams[3] = { inStream, ivStream, keyStream };
  UINT64 ivSize = sizeof(_iv);
  UINT64 keySize = sizeof(_key.Key);
  const UINT64 *inSizes[3] = { inSize, &ivSize, &ivSize, };
  return _aesDecoder->Code(inStreams, inSizes, 3, 
      &outStream, &outSize, 1, progress);
}

}}
