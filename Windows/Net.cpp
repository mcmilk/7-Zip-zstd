// Windows/Net.cpp

#include "StdAfx.h"

#include "Windows/Net.h"

namespace NWindows {
namespace NNet {

DWORD CEnum::Open(DWORD scope, DWORD type, DWORD usage, LPNETRESOURCE netResource)
{
  Close();
  DWORD result = ::WNetOpenEnum(scope, type, usage, netResource, &_handle);
  _handleAllocated = (result == NO_ERROR);
  return result;
}

static void SetComplexString(bool &defined, CSysString &destString, 
    LPCTSTR srsString)
{
  defined = (srsString != 0);
  if (defined)
    destString = srsString;
  else
    destString.Empty();
}

static void ConvertNETRESOURCEToCResource(const NETRESOURCE &netResource, 
    CResource &resource)
{
  resource.Scope = netResource.dwScope;
  resource.Type = netResource.dwType;
  resource.DisplayType = netResource.dwDisplayType;
  resource.Usage = netResource.dwUsage;
  SetComplexString(resource.LocalNameIsDefined, resource.LocalName, netResource.lpLocalName);
  SetComplexString(resource.RemoteNameIsDefined, resource.RemoteName, netResource.lpRemoteName);
  SetComplexString(resource.CommentIsDefined, resource.Comment, netResource.lpComment);
  SetComplexString(resource.ProviderIsDefined, resource.Provider, netResource.lpProvider);
}

static void SetComplexString2(LPTSTR *destString, bool defined, 
    const CSysString &srcString)
{
  if (defined)
    *destString = (TCHAR *)(const TCHAR *)srcString;
  else
    *destString = 0;
}

static void ConvertCResourceToNETRESOURCE(const CResource &resource, 
    NETRESOURCE &netResource)
{
  netResource.dwScope = resource.Scope;
  netResource.dwType = resource.Type;
  netResource.dwDisplayType = resource.DisplayType;
  netResource.dwUsage = resource.Usage;
  SetComplexString2(&netResource.lpLocalName, resource.LocalNameIsDefined, resource.LocalName);
  SetComplexString2(&netResource.lpRemoteName, resource.RemoteNameIsDefined, resource.RemoteName);
  SetComplexString2(&netResource.lpComment, resource.CommentIsDefined, resource.Comment);
  SetComplexString2(&netResource.lpProvider, resource.ProviderIsDefined, resource.Provider);
}

DWORD CEnum::Open(DWORD scope, DWORD type, DWORD usage, 
    const CResource *resource)
{
  NETRESOURCE netResource;
  LPNETRESOURCE pointer;
  if (resource == 0)
    pointer = 0;
  else
  {
    ConvertCResourceToNETRESOURCE(*resource, netResource);
    pointer = &netResource;
  }
  return Open(scope, type, usage, pointer);
}

DWORD CEnum::Close()
{
  if(!_handleAllocated)
    return NO_ERROR;
  DWORD result = ::WNetCloseEnum(_handle);
  _handleAllocated = (result != NO_ERROR);
  return result;
}

DWORD CEnum::Next(LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize)
{
  return ::WNetEnumResource(_handle, lpcCount, lpBuffer, lpBufferSize);
}

DWORD CEnum::Next(CResource &resource)
{
  CByteBuffer byteBuffer;
  const DWORD kBufferSize = 16384;
  byteBuffer.SetCapacity(kBufferSize);
  LPNETRESOURCE lpnrLocal = (LPNETRESOURCE) (BYTE *)(byteBuffer);
  ZeroMemory(lpnrLocal, kBufferSize);
  DWORD bufferSize = kBufferSize;
  DWORD numEntries = 1;
  DWORD result = Next(&numEntries,  lpnrLocal, &bufferSize);
  if (result != NO_ERROR)
    return result;
  if (numEntries != 1)
    return (DWORD)E_FAIL;
  ConvertNETRESOURCEToCResource(lpnrLocal[0], resource);
  return result;
}

DWORD GetResourceParent(const CResource &resource, CResource &parentResource)
{
  CByteBuffer byteBuffer;
  const DWORD kBufferSize = 16384;
  byteBuffer.SetCapacity(kBufferSize);
  LPNETRESOURCE lpnrLocal = (LPNETRESOURCE) (BYTE *)(byteBuffer);
  ZeroMemory(lpnrLocal, kBufferSize);
  DWORD bufferSize = kBufferSize;
  NETRESOURCE netResource;
  ConvertCResourceToNETRESOURCE(resource, netResource);
  DWORD result = ::WNetGetResourceParent(&netResource,  lpnrLocal, &bufferSize);
  if (result != NO_ERROR)
    return result;
  ConvertNETRESOURCEToCResource(lpnrLocal[0], parentResource);
  return result;
}

DWORD GetResourceInformation(const CResource &resource, 
    CResource &destResource, CSysString &systemPathPart)
{
  CByteBuffer byteBuffer;
  const DWORD kBufferSize = 16384;
  byteBuffer.SetCapacity(kBufferSize);
  LPNETRESOURCE lpnrLocal = (LPNETRESOURCE) (BYTE *)(byteBuffer);
  ZeroMemory(lpnrLocal, kBufferSize);
  DWORD bufferSize = kBufferSize;
  NETRESOURCE netResource;
  ConvertCResourceToNETRESOURCE(resource, netResource);
  LPTSTR lplpSystem; 
  DWORD result = ::WNetGetResourceInformation(&netResource, 
      lpnrLocal, &bufferSize, &lplpSystem);
  if (result != NO_ERROR)
    return result;
  if (lplpSystem != 0)
    systemPathPart = lplpSystem;
  ConvertNETRESOURCEToCResource(lpnrLocal[0], destResource);
  return result;
}

DWORD AddConnection2(const CResource &resource, 
    LPCTSTR password, LPCTSTR userName, DWORD flags)
{
  NETRESOURCE netResource;
  ConvertCResourceToNETRESOURCE(resource, netResource);
  return ::WNetAddConnection2(&netResource,
    password, userName, flags);
}

}}
