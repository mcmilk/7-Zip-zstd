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

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

STDMETHODIMP CAgentFolder::GetAgentFolder(CAgentFolder **agentFolder)
{ 
  *agentFolder = this; 
  return S_OK; 
}

STDMETHODIMP CAgentFolder::LoadItems()
{
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = _proxyFolderItem->m_FolderSubItems.Size() +
      _proxyFolderItem->m_FileSubItems.Size();
  return S_OK;
}

/*
STDMETHODIMP CAgentFolder::GetNumberOfSubFolders(UINT32 *aNumSubFolders)
{
  *aNumSubFolders = _proxyFolderItem->m_FolderSubItems.Size();
  return S_OK;
}
*/

STDMETHODIMP CAgentFolder::GetProperty(UINT32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant propVariant;
  if (itemIndex < _proxyFolderItem->m_FolderSubItems.Size())
  {
    const CFolderItem &item = _proxyFolderItem->m_FolderSubItems[itemIndex];
    switch(propID)
    {
      case kpidIsFolder:
        propVariant = true;
        break;
      case kpidName:
        propVariant = item.m_Name;
        break;
      default:
        if (item.m_IsLeaf)
          return _proxyHandler->_archiveHandler->GetProperty(item.m_Index,
              propID, value);
    }
  }
  else
  {
    itemIndex -= _proxyFolderItem->m_FolderSubItems.Size();
    const CFileItem &item = _proxyFolderItem->m_FileSubItems[itemIndex];
    switch(propID)
    {
      case kpidIsFolder:
        propVariant = false;
        break;
      case kpidName:
        propVariant = item.m_Name;
        break;
      default:
        return _proxyHandler->_archiveHandler->GetProperty(item.m_Index,
          propID, value);
    }
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP CAgentFolder::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  if (index >= _proxyFolderItem->m_FolderSubItems.Size())
    return E_INVALIDARG;
  CComObjectNoLock<CAgentFolder> *folderSpec = new CComObjectNoLock<CAgentFolder>;
  CComPtr<IFolderFolder> agentFolder = folderSpec;
  folderSpec->Init(_proxyHandler, &_proxyFolderItem->m_FolderSubItems[index], 
      this, _agentSpec);
  *resultFolder = agentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  int index = _proxyFolderItem->FindDirSubItemIndex(name);
  if (index < 0)
    return E_INVALIDARG;
  return BindToFolder(index, resultFolder);
}

STDMETHODIMP CAgentFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  CComPtr<IFolderFolder> parentFolder = _parentFolder;
  *resultFolder = parentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetName(BSTR *name)
{
  CComBSTR temp = _proxyFolderItem->m_Name;
  *name = temp.Detach();
  return S_OK;
}


#ifdef NEW_FOLDER_INTERFACE

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
  STDMETHOD(Clone) (IEnumSTATPROPSTG **tempEnumerator);
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

  for(DWORD index = 0; index < aNumItems; index++, m_Index++)
  {
    if(m_Index >= m_Properties.Size())
    {
      aResult =  S_FALSE;
      break;
    }
    const CArchiveItemPropertyTemp &aSrcItem = m_Properties[m_Index];
    STATPROPSTG &aDestItem = anItems[index];
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
    *aNumFetched = index;
  return aResult;
}

STDMETHODIMP CEnumFolderItemProperty::Skip(ULONG aNumSkip)
  {  return E_NOTIMPL; }

STDMETHODIMP CEnumFolderItemProperty::Clone(IEnumSTATPROPSTG **tempEnumerator)
  {  return E_NOTIMPL; }


STDMETHODIMP CAgentFolder::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  CComPtr<IEnumSTATPROPSTG> anEnumPropertyArchive;
  RETURN_IF_NOT_S_OK(_proxyHandler->_archiveHandler->EnumProperties(
      &anEnumPropertyArchive));
  
  CComObjectNoLock<CEnumFolderItemProperty> *enumeratorSpec = 
        new CComObjectNoLock<CEnumFolderItemProperty>;
  CComPtr<IEnumSTATPROPSTG> tempEnumerator(enumeratorSpec);

  STATPROPSTG aSrcProperty;
  while (anEnumPropertyArchive->Next(1, &aSrcProperty, NULL) == S_OK)
  {
    CArchiveItemPropertyTemp aDestProperty;
    aDestProperty.Type = aSrcProperty.vt;
    aDestProperty.ID = aSrcProperty.propid;
    if (aDestProperty.ID == kpidPath)
      aDestProperty.ID = kpidName;
    UINT propID = aSrcProperty.propid;
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
    enumeratorSpec->m_Properties.Add(aDestProperty);
  }
  *enumerator = tempEnumerator.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetTypeID(BSTR *name)
{
  UString temp = UString(L"7-Zip.") + NCOM::GUIDToStringW(_agentSpec->_CLSID);
  CComBSTR bstrTemp = temp;
  *name = bstrTemp.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetPath(BSTR *path)
{
  UStringVector pathParts;
  pathParts.Clear();
  CComPtr<IFolderFolder> currentFolder = this;
  while (true)
  {
    CComPtr<IFolderFolder> newFolder;
    currentFolder->BindToParentFolder(&newFolder);  
    if (newFolder == NULL)
      break;
    CComBSTR aName;
    currentFolder->GetName(&aName);
    pathParts.Insert(0, (const wchar_t *)aName);
    currentFolder = newFolder;
  }

  UString prefix;
  for(int i = 0; i < pathParts.Size(); i++)
  {
    prefix += pathParts[i];
    prefix += L'\\';
  }

  CComBSTR tempPath = prefix;
  *path = tempPath.Detach();
  return S_OK;
}
#endif


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
  UStringVector pathParts;
  CFolderItem *aCurrentProxyFolder = _proxyFolderItem;
  while (aCurrentProxyFolder->m_Parent)
  {
    pathParts.Insert(0, aCurrentProxyFolder->m_Name);
    aCurrentProxyFolder = aCurrentProxyFolder->m_Parent;
  }
  anExtractCallBackSpec->Init(_proxyHandler->_archiveHandler, 
      anExtractCallBack2, 
      GetSystemString(aPath, aCodePage),
      aPathMode, 
      anOverwriteMode, 
      pathParts, 
      aCodePage, 
      _proxyHandler->_itemDefaultName,
      _proxyHandler->_defaultTime, 
      _proxyHandler->_defaultAttributes);
  std::vector<UINT32> aRealIndexes;
  _proxyHandler->GetRealIndexes(*_proxyFolderItem, anIndexes, aNumItems, aRealIndexes);
  return _proxyHandler->_archiveHandler->Extract(&aRealIndexes.front(), 
      aRealIndexes.size(), aTestMode, anExtractCallBack);
}


/////////////////////////////////////////
// CAgent

CAgent::CAgent():
  _formatsLoaded(false), _proxyHandler(NULL)
{
}

CAgent::~CAgent()
{
  if (_proxyHandler != NULL)
    delete _proxyHandler;
}

STDMETHODIMP CAgent::Open(IInStream *stream, 
    const wchar_t *defaultName,
    const FILETIME *defaultTime,
    UINT32 defaultAttributes,
    const UINT64 *maxCheckStartPosition,
    const CLSID *aCLSID, 
    IOpenArchive2CallBack *openArchiveCallback)
{
  _defaultName = defaultName;
  _defaultTime = *defaultTime;
  _defaultAttributes = defaultAttributes;
  _CLSID = *aCLSID;

  // m_RootFolder.Release();
  _archive.Release();
  
  #ifdef EXCLUDE_COM
  
  #ifdef FORMAT_7Z
  if (aCLSID->Data4[5] == 5)
    _archive = new CComObjectNoLock<NArchive::N7z::CHandler>;
  #endif
  
  if (!_archive)
    return E_FAIL;

  #else
  
  if (_archive.CoCreateInstance(*aCLSID) != S_OK)
    return S_FALSE;
  
  #endif

  HRESULT aResult = _archive->Open(stream, maxCheckStartPosition, openArchiveCallback);
  if (aResult != S_OK)
    _archive.Release();
  return aResult;
}

STDMETHODIMP CAgent::ReOpen(IInStream *stream, 
      const wchar_t *defaultName,
      const FILETIME *defaultTime,
      UINT32 defaultAttributes,
      const UINT64 *maxCheckStartPosition,
      IOpenArchive2CallBack *openArchiveCallback)
{
  _defaultName = defaultName;
  _defaultTime = *defaultTime;
  _defaultAttributes = defaultAttributes;

  delete _proxyHandler;
  _proxyHandler = 0;
  HRESULT aResult = _archive->Open(stream, maxCheckStartPosition, openArchiveCallback);
  if (aResult != S_OK)
    _archive.Release();
  return aResult;
}

STDMETHODIMP CAgent::Close()
{
  return _archive->Close();
}

STDMETHODIMP CAgent::EnumProperties(IEnumSTATPROPSTG **EnumProperties)
{
  return _archive->EnumProperties(EnumProperties);
}

HRESULT CAgent::ReadItems()
{
  if (_proxyHandler != NULL)
    return S_OK;
  _proxyHandler = new CAgentProxyHandler();
  RETURN_IF_NOT_S_OK(_proxyHandler->Init(_archive, _defaultName, 
    _defaultTime, _defaultAttributes, NULL));
  return S_OK;
}

STDMETHODIMP CAgent::BindToRootFolder(IFolderFolder **resultFolder)
{
  RETURN_IF_NOT_S_OK(ReadItems());
  CComObjectNoLock<CAgentFolder> *folderSpec = new CComObjectNoLock<CAgentFolder>;
  CComPtr<IFolderFolder> rootFolder = folderSpec;
  folderSpec->Init(_proxyHandler, &_proxyHandler->_folderItemHead, NULL, this);
  *resultFolder = rootFolder.Detach();
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
  anExtractCallBackSpec->Init(_archive, 
      anExtractCallBack2, 
      GetSystemString(aPath, aCodePage),
      aPathMode, 
      anOverwriteMode, 
      UStringVector(), 
      aCodePage, 
      _defaultName,
      _defaultTime, 
      _defaultAttributes);
  return _archive->ExtractAllItems(aTestMode, anExtractCallBack);
}


