// HandlersManager.h

#pragma once

#ifndef __HANDLERSMANAGER_H
#define __HANDLERSMANAGER_H

#include "ProxyHandler.h"
#include "Windows/COM.h"
#include "../Common/ZipRegistry.h"

struct CHandlerPair
{
  CSysString FileName;
  CComPtr<IArchiveHandler100> ArchiveHandler;
  CProxyHandler *ProxyHandler;
  // UINT64 Cookie;
};

class CHandlersManager
{
  UINT64 m_Cookie;

  NWindows::NCOM::CComInitializer m_ComInitializer;
  //CPointerVector m_ProxyHandlers;
  CObjectVector<CHandlerPair> m_HandlerPairs;
 
  CZipRegistryManager m_ZipRegistryManager;

 
public:
  bool AreThereHandlers() const { return m_HandlerPairs.Size() > 0; }
  bool GetProxyHandler(const CSysString &aFileName, 
      bool anOpenAlways,
      CProxyHandler **aProxyHandler,
      IUnknown **aProxyHandlerRef,
      IArchiveHandler100 **anArchive, 
      bool &aHandlerIsNew/*, UINT64 &aCookie*/);
  // bool GetProxyHandler(const CSysString &aFileName, CHandlerPair &aHandlerPairResult);
  // HRESULT ReOpenProxyHandler(CProxyHandler **aProxyHandler, UINT64 &aCookie);
  void RemoveNotUsedHandlers();
  CHandlersManager();
  ~CHandlersManager();
};

extern CHandlersManager g_HandlersManager;

#endif