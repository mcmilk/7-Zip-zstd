// Windows/Net.cpp

#include "StdAfx.h"

#include "Windows/Net.h"

namespace NWindows {
namespace NNet {

DWORD CEnum::Open(DWORD dwScope, DWORD dwType, DWORD dwUsage, 
    LPNETRESOURCE lpNetResource)
{
  Close();
  DWORD aResult = ::WNetOpenEnum(dwScope, dwType, dwUsage, 
      lpNetResource, &m_Handle);
  m_HandleAllocated = (aResult == NO_ERROR);
  return aResult;
}

static void SetComplexString(bool &aBool, CSysString &aStringTo, LPCTSTR aStringFrom)
{
  aBool = (aStringFrom != 0);
  if (aBool)
    aStringTo = aStringFrom;
  else
    aStringTo.Empty();
}

static void ConvertNETRESOURCEToCResource(const NETRESOURCE &aNETRESOURCE, 
    CResource &aResource)
{
  aResource.Scope = aNETRESOURCE.dwScope;
  aResource.Type = aNETRESOURCE.dwType;
  aResource.DisplayType = aNETRESOURCE.dwDisplayType;
  aResource.Usage = aNETRESOURCE.dwUsage;
  SetComplexString(aResource.LocalNameIsDefined, aResource.LocalName, aNETRESOURCE.lpLocalName);
  SetComplexString(aResource.RemoteNameIsDefined, aResource.RemoteName, aNETRESOURCE.lpRemoteName);
  SetComplexString(aResource.CommentIsDefined, aResource.Comment, aNETRESOURCE.lpComment);
  SetComplexString(aResource.ProviderIsDefined, aResource.Provider, aNETRESOURCE.lpProvider);
}

static void SetComplexString2(LPCTSTR &aStringTo, bool aBool, 
    const CSysString &aStringFrom)
{
  if (aBool)
    aStringTo = aStringFrom;
  else
    aStringTo = 0;
}

static void ConvertCResourceToNETRESOURCE(const CResource &aResource, 
    NETRESOURCE &aNETRESOURCE)
{
  aNETRESOURCE.dwScope = aResource.Scope;
  aNETRESOURCE.dwType = aResource.Type;
  aNETRESOURCE.dwDisplayType = aResource.DisplayType;
  aNETRESOURCE.dwUsage = aResource.Usage;
  SetComplexString2(aNETRESOURCE.lpLocalName, aResource.LocalNameIsDefined, aResource.LocalName);
  SetComplexString2(aNETRESOURCE.lpRemoteName, aResource.RemoteNameIsDefined, aResource.RemoteName);
  SetComplexString2(aNETRESOURCE.lpComment, aResource.CommentIsDefined, aResource.Comment);
  SetComplexString2(aNETRESOURCE.lpProvider, aResource.ProviderIsDefined, aResource.Provider);
}

DWORD CEnum::Open(DWORD dwScope, DWORD dwType, DWORD dwUsage, 
    const CResource *aResource)
{
  NETRESOURCE aNETRESOURCE;
  LPNETRESOURCE aPointer;
  if (aResource == 0)
    aPointer = 0;
  else
  {
    ConvertCResourceToNETRESOURCE(*aResource, aNETRESOURCE);
    aPointer = &aNETRESOURCE;
  }
  return Open(dwScope, dwType, dwUsage, aPointer);
}

DWORD CEnum::Close()
{
  if(!m_HandleAllocated)
    return NO_ERROR;
  DWORD aResult = ::WNetCloseEnum(m_Handle);
  m_HandleAllocated = (aResult != NO_ERROR);
  return aResult;
}

DWORD CEnum::Next(LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize)
{
  return ::WNetEnumResource(m_Handle, lpcCount, lpBuffer, lpBufferSize);
}

DWORD CEnum::Next(CResource &aResource)
{
  CByteBuffer aByteBuffer;
  const DWORD kBufferSize = 16384;
  aByteBuffer.SetCapacity(kBufferSize);
  LPNETRESOURCE lpnrLocal = (LPNETRESOURCE) (BYTE *)(aByteBuffer);
  ZeroMemory(lpnrLocal, kBufferSize);
  DWORD aBufferSize = kBufferSize;
  DWORD cEntries = 1;
  DWORD aResult = Next(&cEntries,  lpnrLocal, &aBufferSize);
  if (aResult != NO_ERROR)
    return aResult;
  if (cEntries != 1)
    return E_FAIL;
  ConvertNETRESOURCEToCResource(lpnrLocal[0], aResource);
  return aResult;
}

DWORD GetResourceParent(const CResource &aResource, CResource &aResourceParent)
{
  CByteBuffer aByteBuffer;
  const DWORD kBufferSize = 16384;
  aByteBuffer.SetCapacity(kBufferSize);
  LPNETRESOURCE lpnrLocal = (LPNETRESOURCE) (BYTE *)(aByteBuffer);
  ZeroMemory(lpnrLocal, kBufferSize);
  DWORD aBufferSize = kBufferSize;
  NETRESOURCE aNETRESOURCE;
  ConvertCResourceToNETRESOURCE(aResource, aNETRESOURCE);
  DWORD aResult = ::WNetGetResourceParent(&aNETRESOURCE,  lpnrLocal, &aBufferSize);
  if (aResult != NO_ERROR)
    return aResult;
  ConvertNETRESOURCEToCResource(lpnrLocal[0], aResourceParent);
  return aResult;
}

DWORD GetResourceInformation(const CResource &aResource, 
    CResource &aDestResource, CSysString &aSystemPathPart)
{
  CByteBuffer aByteBuffer;
  const DWORD kBufferSize = 16384;
  aByteBuffer.SetCapacity(kBufferSize);
  LPNETRESOURCE lpnrLocal = (LPNETRESOURCE) (BYTE *)(aByteBuffer);
  ZeroMemory(lpnrLocal, kBufferSize);
  DWORD aBufferSize = kBufferSize;
  NETRESOURCE aNETRESOURCE;
  ConvertCResourceToNETRESOURCE(aResource, aNETRESOURCE);
  LPTSTR lplpSystem; 
  DWORD aResult = ::WNetGetResourceInformation(&aNETRESOURCE, 
      lpnrLocal, &aBufferSize, &lplpSystem);
  if (aResult != NO_ERROR)
    return aResult;
  if (lplpSystem != 0)
    aSystemPathPart = lplpSystem;
  ConvertNETRESOURCEToCResource(lpnrLocal[0], aDestResource);
  return aResult;
}

DWORD AddConnection2(const CResource &aResource, 
    LPCTSTR lpPassword, LPCTSTR lpUsername, DWORD dwFlags)
{
  NETRESOURCE aNETRESOURCE;
  ConvertCResourceToNETRESOURCE(aResource, aNETRESOURCE);
  return ::WNetAddConnection2(&aNETRESOURCE,
    lpPassword, lpUsername, dwFlags);
}


}}


