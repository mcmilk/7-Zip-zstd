// Windows/Net.h

#pragma once

#ifndef __WINDOWS_NET_H
#define __WINDOWS_NET_H

#include "Common/Buffer.h"
#include "Common/String.h"

namespace NWindows {
namespace NNet {

struct CResource
{
  DWORD Scope; 
  DWORD Type; 
  DWORD DisplayType; 
  DWORD Usage; 
  bool LocalNameIsDefined;
  bool RemoteNameIsDefined;
  bool CommentIsDefined;
  bool ProviderIsDefined;
  CSysString LocalName;
  CSysString RemoteName;
  CSysString Comment;
  CSysString Provider;
};

class CEnum
{
  HANDLE _handle;
  bool _handleAllocated;
protected:
  bool IsHandleAllocated() const { return _handleAllocated; }
public:
  CEnum(): _handleAllocated(false) {}
  ~CEnum() {  Close(); }
  DWORD Open(DWORD scope, DWORD type, DWORD usage, LPNETRESOURCE netResource);
  DWORD Open(DWORD scope, DWORD type, DWORD usage, const CResource *resource);
  DWORD Close();
  DWORD Next(LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize);
  DWORD Next(CResource &resource);
};

DWORD GetResourceParent(const CResource &resource, CResource &parentResource);
DWORD GetResourceInformation(const CResource &resource, 
    CResource &destResource, CSysString &systemPathPart);
DWORD AddConnection2(const CResource &resource, 
    LPCTSTR password, LPCTSTR userName, DWORD flags);

}}

#endif
