// Zip/Handler.cpp

#include "StdAfx.h"

#include "Handler.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/COM.h"

#include "ExtractCallback200.h"
#include "Common/StringConvert.h"

#ifdef FORMAT_7Z
#include "../Format/7z/Handler.h"
#endif

using namespace NWindows;

struct CArchiveItemPropertyTemp
{
  UString Name;
  PROPID ID;
  VARTYPE Type;
};


class CEnumFolderItemProperty:
  public IEnumSTATPROPSTG,
  public CComObjectRoot
{
public:
  int m_Index;
  CObjectVector<CArchiveItemPropertyTemp> m_Properties;

  BEGIN_COM_MAP(CEnumFolderItemProperty)
    COM_INTERFACE_ENTRY(IEnumSTATPROPSTG)
  END_COM_MAP()
    
  DECLARE_NOT_AGGREGATABLE(CEnumFolderItemProperty)
    
  DECLARE_NO_REGISTRY()
public:
  CEnumFolderItemProperty(): m_Index(0) {};

  STDMETHOD(Next) (ULONG aNumItems, STATPROPSTG *anItems, ULONG *aNumFetched);
  STDMETHOD(Skip)  (ULONG aNumItems);
  STDMETHOD(Reset) ();
  STDMETHOD(Clone) (IEnumSTATPROPSTG **anEnum);
};

STDMETHODIMP CEnumFolderItemProperty::Reset()
{
  m_Index = 0;
  return S_OK;
}

STDMETHODIMP CEnumFolderItemProperty::Next(ULONG aNumItems, 
    STATPROPSTG *anItems, ULONG *aNumFetched)
{
  HRESULT aResult = S_OK;
  if(aNumItems > 1 && !aNumFetched)
    return E_INVALIDARG;

  for(DWORD anIndex = 0; anIndex < aNumItems; anIndex++, m_Index++)
  {
    if(m_Index >= m_Properties.Size())
    {
      aResult =  S_FALSE;
      break;
    }
    const CArchiveItemPropertyTemp &aSrcItem = m_Properties[m_Index];
    STATPROPSTG &aDestItem = anItems[anIndex];
    aDestItem.propid = aSrcItem.ID;
    aDestItem.vt = aSrcItem.Type;
    if(!aSrcItem.Name.IsEmpty())
    {
      aDestItem.lpwstrName = (wchar_t *)CoTaskMemAlloc((wcslen(aSrcItem.Name) + 1) * sizeof(wchar_t));
      wcscpy(aDestItem.lpwstrName, aSrcItem.Name);
    }
    else
      aDestItem.lpwstrName = 0; // aSrcItem.lpwstrName;
  }
  if (aNumFetched)
    *aNumFetched = anIndex;
  return aResult;
}

STDMETHODIMP CEnumFolderItemProperty::Skip(ULONG aNumSkip)
  {  return E_NOTIMPL; }

STDMETHODIMP CEnumFolderItemProperty::Clone(IEnumSTATPROPSTG **anEnum)
  {  return E_NOTIMPL; }


#ifdef NEW_FOLDER_INTERFACE

STDMETHODIMP CAgentFolder::EnumProperties(IEnumSTATPROPSTG **anEnumProperty)
{
  CComPtr<IEnumSTATPROPSTG> anEnumPropertyArchive;
  RETURN_IF_NOT_S_OK(m_ProxyHandler->m_ArchiveHandler->EnumProperties(
      &anEnumPropertyArchive));
  
  CComObjectNoLock<CEnumFolderItemProperty> *anEnumObject = 
        new CComObjectNoLock<CEnumFolderItemProperty>;
  CComPtr<IEnumSTATPROPSTG> anEnum(anEnumObject);

  STATPROPSTG aSrcProperty;
  while (anEnumPropertyArchive->Next(1, &aSrcProperty, NULL) == S_OK)
  {
    CArchiveItemPropertyTemp aDestProperty;
    aDestProperty.Type = aSrcProperty.vt;
    aDestProperty.ID = aSrcProperty.propid;
    if (aDestProperty.ID == kaipidPath)
      aDestProperty.ID = kaipidName;
    UINT aPropID = aSrcProperty.propid;
    AString aPropName;
    {
      if (aSrcProperty.lpwstrName != NULL)
        aDestProperty.Name = aSrcProperty.lpwstrName;
      /*
      else
        aDestProperty.Name = "Error";
      */
    }
    if (aSrcProperty.lpwstrName != NULL)
      CoTaskMemFree(aSrcProperty.lpwstrName);
    anEnumObject->m_Properties.Add(aDestProperty);
  }
  *anEnumProperty = anEnum.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetTypeID(BSTR *aName)
{
  UString aNameTemp = UString(L"Archive") + 
      NCOM::GUIDToStringW(m_AgentSpec->m_CLSID);
  CComBSTR aBSTRName = aNameTemp;
  *aName = aBSTRName.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetPath(BSTR *aPath)
{
  UStringVector aPathParts;
  aPathParts.Clear();
  CComPtr<IArchiveFolder> aFolderItem = this;
  while (true)
  {
    CComPtr<IArchiveFolder> aNewFolder;
    aFolderItem->BindToParentFolder(&aNewFolder);  
    if (aNewFolder == NULL)
      break;
    CComBSTR aName;
    aFolderItem->GetName(&aName);
    aPathParts.Insert(0, (const wchar_t *)aName);
    aFolderItem = aNewFolder;
  }

  UString aPrefix;
  for(int i = 0; i < aPathParts.Size(); i++)
  {
    aPrefix += aPathParts[i];
    aPrefix += L'\\';
  }

  CComBSTR aBSTRName = aPrefix;
  *aPath = aBSTRName.Detach();
  return S_OK;
}
#endif

STDMETHODIMP CAgentFolder::GetAgentFolder(CAgentFolder **anAgentFolder)
{ 
  *anAgentFolder = this; 
  return S_OK; 
}

STDMETHODIMP CAgentFolder::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = m_ProxyFolderItem->m_FolderSubItems.Size() +
      m_ProxyFolderItem->m_FileSubItems.Size();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetNumberOfSubFolders(UINT32 *aNumSubFolders)
{
  *aNumSubFolders = m_ProxyFolderItem->m_FolderSubItems.Size();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetProperty(UINT32 anItemIndex, PROPID aPropID, PROPVARIANT *aValue)
{
  NCOM::CPropVariant aPropVariant;
  if (anItemIndex < m_ProxyFolderItem->m_FolderSubItems.Size())
  {
    const CFolderItem &anItem = m_ProxyFolderItem->m_FolderSubItems[anItemIndex];
    switch(aPropID)
    {
      case kaipidIsFolder:
        aPropVariant = true;
        break;
      case kaipidName:
        aPropVariant = anItem.m_Name;
        break;
      default:
        if (anItem.m_IsLeaf)
          return m_ProxyHandler->m_ArchiveHandler->GetProperty(anItem.m_Index,
              aPropID, aValue);
    }
  }
  else
  {
    anItemIndex -= m_ProxyFolderItem->m_FolderSubItems.Size();
    const CFileItem &anItem = m_ProxyFolderItem->m_FileSubItems[anItemIndex];
    switch(aPropID)
    {
      case kaipidIsFolder:
        aPropVariant = false;
        break;
      case kaipidName:
        aPropVariant = anItem.m_Name;
        break;
      default:
        return m_ProxyHandler->m_ArchiveHandler->GetProperty(anItem.m_Index,
          aPropID, aValue);
    }
  }
  aPropVariant.Detach(aValue);
  return S_OK;
}

STDMETHODIMP CAgentFolder::BindToFolder(UINT32 anIndex, IArchiveFolder **aFolder)
{
  if (anIndex >= m_ProxyFolderItem->m_FolderSubItems.Size())
    return E_INVALIDARG;
  CComObjectNoLock<CAgentFolder> *anAgentFolderSpec = new 
      CComObjectNoLock<CAgentFolder>;
  CComPtr<IArchiveFolder> anAgentFolder = anAgentFolderSpec;
  anAgentFolderSpec->Init(m_ProxyHandler, 
      &m_ProxyFolderItem->m_FolderSubItems[anIndex], this, m_AgentSpec);
  *aFolder = anAgentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::BindToFolder(const WCHAR *aFolderName, IArchiveFolder **aFolder)
{
  int anIndex = m_ProxyFolderItem->FindDirSubItemIndex(aFolderName);
  if (anIndex < 0)
    return E_INVALIDARG;
  return BindToFolder(anIndex, aFolder);
}

STDMETHODIMP CAgentFolder::BindToParentFolder(IArchiveFolder **aFolder)
{
  CComPtr<IArchiveFolder> aParentFolder = m_ParentFolder;
  *aFolder = aParentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetName(BSTR *aName)
{
  CComBSTR aBSTRName = m_ProxyFolderItem->m_Name;
  *aName = aBSTRName.Detach();
  return S_OK;
}


static inline UINT GetCurrentFileCodePage()
{
  return AreFileApisANSI() ? CP_ACP : CP_OEMCP;
}

STDMETHODIMP CAgentFolder::Extract(const UINT32 *anIndexes, 
    UINT32 aNumItems, 
    NExtractionMode::NPath::EEnum aPathMode, 
    NExtractionMode::NOverwrite::EEnum anOverwriteMode, 
    const wchar_t *aPath,
    INT32 aTestMode,
    IExtractCallback2 *anExtractCallBack2)
{
  UINT aCodePage = GetCurrentFileCodePage();
  CComObjectNoLock<CExtractCallBack200Imp> *anExtractCallBackSpec = new 
      CComObjectNoLock<CExtractCallBack200Imp>;
  CComPtr<IExtractCallback200> anExtractCallBack = anExtractCallBackSpec;
  UStringVector aPathParts;
  CFolderItem *aCurrentProxyFolder = m_ProxyFolderItem;
  while (aCurrentProxyFolder->m_Parent)
  {
    aPathParts.Insert(0, aCurrentProxyFolder->m_Name);
    aCurrentProxyFolder = aCurrentProxyFolder->m_Parent;
  }
  anExtractCallBackSpec->Init(m_ProxyHandler->m_ArchiveHandler, 
      anExtractCallBack2, 
      GetSystemString(aPath, aCodePage),
      aPathMode, 
      anOverwriteMode, 
      aPathParts, 
      aCodePage, 
      m_ProxyHandler->m_ItemDefaultName,
      m_ProxyHandler->m_DefaultTime, 
      m_ProxyHandler->m_DefaultAttributes, 
      false, 
      L"");
  std::vector<UINT32> aRealIndexes;
  m_ProxyHandler->GetRealIndexes(*m_ProxyFolderItem, anIndexes, aNumItems, aRealIndexes);
  return m_ProxyHandler->m_ArchiveHandler->Extract(&aRealIndexes.front(), 
      aRealIndexes.size(), aTestMode, anExtractCallBack);
}


/////////////////////////////////////////
// CAgent

CAgent::CAgent():
  m_ProxyHandler(NULL)
{
}

CAgent::~CAgent()
{
  if (m_ProxyHandler != NULL)
    delete m_ProxyHandler;
}

STDMETHODIMP CAgent::Open(IInStream *aStream, 
    const wchar_t *aDefaultName,
    const FILETIME *aDefaultTime,
    UINT32 aDefaultAttributes,
    const UINT64 *aMaxCheckStartPosition,
    const CLSID *aCLSID, 
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  m_DefaultName = aDefaultName;
  m_DefaultTime = *aDefaultTime;
  m_DefaultAttributes = aDefaultAttributes;
  m_CLSID = *aCLSID;

  // m_RootFolder.Release();
  m_Archive.Release();
  
  #ifdef EXCLUDE_COM
  
  #ifdef FORMAT_7Z
  if (aCLSID->Data4[5] == 5)
    m_Archive = new CComObjectNoLock<NArchive::N7z::CHandler>;
  #endif
  
  if (!m_Archive)
    return E_FAIL;

  #else
  
  if (m_Archive.CoCreateInstance(*aCLSID) != S_OK)
    return S_FALSE;
  
  #endif

  HRESULT aResult = m_Archive->Open(aStream, aMaxCheckStartPosition, anOpenArchiveCallBack);
  if (aResult != S_OK)
    m_Archive.Release();
  return aResult;
}

STDMETHODIMP CAgent::ReOpen(IInStream *aStream, 
      const wchar_t *aDefaultName,
      const FILETIME *aDefaultTime,
      UINT32 aDefaultAttributes,
      const UINT64 *aMaxCheckStartPosition,
      IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  m_DefaultName = aDefaultName;
  m_DefaultTime = *aDefaultTime;
  m_DefaultAttributes = aDefaultAttributes;

  delete m_ProxyHandler;
  m_ProxyHandler = 0;
  HRESULT aResult = m_Archive->Open(aStream, aMaxCheckStartPosition, anOpenArchiveCallBack);
  if (aResult != S_OK)
    m_Archive.Release();
  return aResult;
}

STDMETHODIMP CAgent::Close()
{
  return m_Archive->Close();
}

STDMETHODIMP CAgent::EnumProperties(IEnumSTATPROPSTG **EnumProperties)
{
  return m_Archive->EnumProperties(EnumProperties);
}

HRESULT CAgent::ReadItems()
{
  if (m_ProxyHandler != NULL)
    return S_OK;
  m_ProxyHandler = new CAgentProxyHandler();
  RETURN_IF_NOT_S_OK(m_ProxyHandler->Init(m_Archive, m_DefaultName, 
    m_DefaultTime, m_DefaultAttributes, NULL));
  return S_OK;
}

STDMETHODIMP CAgent::BindToRootFolder(IArchiveFolder **aFolder)
{
  RETURN_IF_NOT_S_OK(ReadItems());
  CComObjectNoLock<CAgentFolder> *anAgentFolderSpec = new 
    CComObjectNoLock<CAgentFolder>;
  anAgentFolderSpec->Init(m_ProxyHandler, &m_ProxyHandler->m_FolderItemHead, NULL, this);
  CComPtr<IArchiveFolder> aRootFolder = anAgentFolderSpec;
  *aFolder = aRootFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgent::Extract(
    NExtractionMode::NPath::EEnum aPathMode, 
    NExtractionMode::NOverwrite::EEnum anOverwriteMode, 
    const wchar_t *aPath,
    INT32 aTestMode,
    IExtractCallback2 *anExtractCallBack2)
{
  UINT aCodePage = GetCurrentFileCodePage();
  CComObjectNoLock<CExtractCallBack200Imp> *anExtractCallBackSpec = new 
      CComObjectNoLock<CExtractCallBack200Imp>;
  CComPtr<IExtractCallback200> anExtractCallBack = anExtractCallBackSpec;
  anExtractCallBackSpec->Init(m_Archive, 
      anExtractCallBack2, 
      GetSystemString(aPath, aCodePage),
      aPathMode, 
      anOverwriteMode, 
      UStringVector(), 
      aCodePage, 
      m_DefaultName,
      m_DefaultTime, 
      m_DefaultAttributes, 
      false, 
      L"");
  return m_Archive->ExtractAllItems(aTestMode, anExtractCallBack);
}


