// HandlerManager.h

#include "StdAfx.h"

#include "HandlersManager.h"

#include "../Common/OpenEngine2.h"

#include "Windows/Synchronization.h"

#include "..\..\Archiver\Common\DefaultName.h"

using namespace NWindows;

CHandlersManager g_HandlersManager;

CHandlersManager::CHandlersManager()
{
  m_Cookie = 0;
  // MessageBox(NULL, "CHandlersManager::CHandlersManager", "", MB_OK);
}

CHandlersManager::~CHandlersManager()
{
  // MessageBox(NULL, "CHandlersManager::~CHandlersManager", "", MB_OK);
  // TRACE0("~CHandlersManager\n");
  while(!m_HandlerPairs.IsEmpty())
  {
    {
      CHandlerPair &aHandlerPair = m_HandlerPairs.Front();
      CProxyHandler * aProxyHandler = aHandlerPair.ProxyHandler;
      aHandlerPair.ArchiveHandler.Detach(); // wor 98, siznce dll unloaded by system.
      int aRefCount = aProxyHandler->m_dwRef;
      for(int i = 0; i < aRefCount; i++)
        aProxyHandler->Release();
    }
    m_HandlerPairs.Delete(0);
  }
}

NSynchronization::CCriticalSection g_RemoveProxyCriticalSection;

void CHandlersManager::RemoveNotUsedHandlers()
{
  NSynchronization::CSingleLock aLock(&g_RemoveProxyCriticalSection, true);
  for(int i = 0; i < m_HandlerPairs.Size();)
  {
    CHandlerPair *aHandlerPair = &m_HandlerPairs[i];
    CProxyHandler * aProxyHandler = aHandlerPair->ProxyHandler;
    if (aProxyHandler->m_dwRef == 1)
    {
      aProxyHandler->Release();
      aHandlerPair->ArchiveHandler.Release();
      m_HandlerPairs.Delete(i);
    }
    else
      i++;
  }
}

extern void DeleteOldTempFiles();

NSynchronization::CCriticalSection g_ChangeProxyListCriticalSection;

bool CHandlersManager::GetProxyHandler(const CSysString &aFileName, 
    bool anOpenAlways,
    CProxyHandler **aHandlerResult, 
    IUnknown **aProxyHandlerRef,
    IArchiveHandler100 **anArchiveResult, 
    bool &aHandlerIsNew/*, UINT64 &aCookie*/)
{
  *aHandlerResult = NULL;
  NSynchronization::CSingleLock aLock(&g_ChangeProxyListCriticalSection, true);
  RemoveNotUsedHandlers();
  CComPtr<IUnknown> aRef;
  for(int i = 0; i < m_HandlerPairs.Size(); i++)
  {
    CHandlerPair &aHandlerPair = m_HandlerPairs[i];
    CProxyHandler *aProxyHandler = aHandlerPair.ProxyHandler;
    if(aHandlerPair.FileName == aFileName)
    {
      aRef = aProxyHandler;
      CComPtr<IArchiveHandler100> aTemp = aHandlerPair.ArchiveHandler;
      *aHandlerResult = aProxyHandler;
      *aProxyHandlerRef = aRef.Detach();
      *anArchiveResult = aTemp.Detach();

      // aCookie = aHandlerPair.Cookie;
      aHandlerIsNew = false;
      return true;
    }
  }
  if (!anOpenAlways)
    return true;

  CComPtr<IArchiveHandler100> anArchiveHandler;
  NZipRootRegistry::CArchiverInfo anArchiverInfoResult;
  UString aDefaultName;
  HRESULT aResult = OpenArchive(aFileName, &anArchiveHandler, anArchiverInfoResult,
      aDefaultName, NULL);
  if (aResult != S_OK)
    return false;

  DeleteOldTempFiles();

  CProxyHandler *aProxyHandler = new CProxyHandler();
  aProxyHandler->AddRef(); // for List itself;
  if(!aProxyHandler->Init(anArchiveHandler, anArchiverInfoResult.ClassID))
  {
    aProxyHandler->Release();
    return false;
  }
  CHandlerPair aHandlerPair;
  aHandlerPair.ProxyHandler = aProxyHandler;
  aHandlerPair.FileName = aFileName;
  aHandlerPair.ArchiveHandler = anArchiveHandler;
  // aHandlerPair.Cookie = m_Cookie++;
  m_HandlerPairs.Add(aHandlerPair);

  aRef = aProxyHandler;
  *aProxyHandlerRef = aRef.Detach();
  // aProxyHandler->AddRef(); // for client
  *aHandlerResult = aProxyHandler;
  *anArchiveResult = anArchiveHandler.Detach();
  // aCookie = aHandlerPair.Cookie;
  aHandlerIsNew = true;
  return true;
}

/*
HRESULT CHandlersManager::ReOpenProxyHandler(CProxyHandler *aProxyHandler, 
    UINT64 &aCookie)
{
  *aProxyHandlerResult = NULL;
  NSynchronization::CSingleLock aLock(&g_ChangeProxyListCriticalSection, true);
  RemoveNotUsedHandlers();
  for(int i = 0; i < m_HandlerPairs.Size(); i++)
  {
    CHandlerPair &aHandlerPair = m_HandlerPairs[i];
    if(aProxyHandler == aHandlerPair.Handler)
    {
      if (aCookie != aHandlerPair.Cookie)



      aProxyHandler->AddRef(); // test it
      aHandlerPair.Index++;
      *aProxyHandlerResult = aProxyHandler;
     return ReOpenArchive(aProxyHandler->m_ArchiveHandler, aFileName);

    }
  }
  return E_FAIL;
}
*/