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
  HANDLE m_Handle;
  bool m_HandleAllocated;
protected:
  bool IsHandleAllocated() const { return m_HandleAllocated; }
public:
  CEnum(): m_HandleAllocated(false) {}
  ~CEnum() {  Close(); }
  DWORD Open(DWORD dwScope, DWORD dwType, DWORD dwUsage, 
      LPNETRESOURCE lpNetResource);
  DWORD Open(DWORD dwScope, DWORD dwType, DWORD dwUsage, 
      const CResource *aNetResource);
  DWORD Close();
  DWORD Next(LPDWORD lpcCount, LPVOID lpBuffer, LPDWORD lpBufferSize);
  DWORD Next(CResource &aResource);
};

DWORD GetResourceParent(const CResource &aResource, CResource &aResourceParent);
DWORD GetResourceInformation(const CResource &aResource, 
    CResource &aDestResource, CSysString &aSystemPathPart);
DWORD AddConnection2(const CResource &aResource, 
    LPCTSTR lpPassword, LPCTSTR lpUsername, DWORD dwFlags);


}}

#endif
