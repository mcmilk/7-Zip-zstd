// ProxyHandler.h

#pragma once

#ifndef __PROXYHANDLER_H
#define __PROXYHANDLER_H

#include "../Common/ZipRegistry.h"
#include "../Common/IArchiveHandler2.h"


struct CArchiveItemProperty
{
  CSysString Name;
  PROPID ID;
  VARTYPE Type;
};

typedef CObjectVector<CArchiveItemProperty> CArchiveItemPropertyVector;

class CProxyHandlerSpec:
  public IUnknown,
  public CComObjectRoot
{
public:
  BEGIN_COM_MAP(CProxyHandlerSpec)
    COM_INTERFACE_ENTRY(IUnknown)
  END_COM_MAP()
    
  DECLARE_NOT_AGGREGATABLE(CProxyHandlerSpec)
    
  DECLARE_NO_REGISTRY()

  CZipRegistryManager m_ZipRegistryManager;
  NZipSettings::CListViewInfo m_ListViewInfo;
  CLSID m_ClassID;
  CSysString m_FileTypePropertyCaption;

  CProxyHandlerSpec();
  ~CProxyHandlerSpec();

  bool Init(IArchiveHandler100 *aHandler, const CLSID &aClassID);

  // CComPtr<IArchiveHandler> m_ArchiveHandler;
  void SaveColumnsInfo(const NZipSettings::CListViewInfo &aViewInfo);
  void ReadColumnsInfo(NZipSettings::CListViewInfo &aViewInfo);

  int FindProperty(PROPID aPropID);
  VARTYPE CProxyHandlerSpec::GetTypeOfProperty(PROPID aPropID);
  CSysString GetNameOfProperty(PROPID aPropID);

  CArchiveItemPropertyVector m_HandlerProperties;
  CArchiveItemPropertyVector m_InternalProperties;
  CArchiveItemPropertyVector m_ColumnsProperties;
private:
  bool ReadProperties(IArchiveHandler100 *aHandler);
};

typedef CComObjectNoLock<CProxyHandlerSpec> CProxyHandler;

#endif