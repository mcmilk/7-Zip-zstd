// Agent.cpp

#include "StdAfx.h"

#include "Agent.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileFind.h"

#include "ArchiveExtractCallback.h"
#include "Common/StringConvert.h"
#include "../Common/OpenArchive.h"
#include "../Common/DefaultName.h"

#ifdef FORMAT_7Z
#include "../../Archive/7z/7zHandler.h"
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

STDMETHODIMP CAgentFolder::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _proxyFolderItem->Folders.Size() +
      _proxyFolderItem->Files.Size();
  return S_OK;
}

/*
STDMETHODIMP CAgentFolder::GetNumberOfSubFolders(UINT32 *aNumSubFolders)
{
  *aNumSubFolders = _proxyFolderItem->Folders.Size();
  return S_OK;
}
*/

STDMETHODIMP CAgentFolder::GetProperty(UINT32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant propVariant;
  if (itemIndex < (UINT32)_proxyFolderItem->Folders.Size())
  {
    const CProxyFolder &item = _proxyFolderItem->Folders[itemIndex];
    switch(propID)
    {
      case kpidIsFolder:
        propVariant = true;
        break;
      case kpidName:
        propVariant = item.Name;
        break;
      default:
        if (item.IsLeaf)
          return _agentSpec->_archive->GetProperty(item.Index,
              propID, value);
    }
  }
  else
  {
    itemIndex -= _proxyFolderItem->Folders.Size();
    const CProxyFile &item = _proxyFolderItem->Files[itemIndex];
    switch(propID)
    {
      case kpidIsFolder:
        propVariant = false;
        break;
      case kpidName:
        propVariant = item.Name;
        break;
      default:
        return _agentSpec->_archive->GetProperty(item.Index,
          propID, value);
    }
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP CAgentFolder::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  if (index >= (UINT32)_proxyFolderItem->Folders.Size())
    return E_INVALIDARG;
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> agentFolder = folderSpec;
  folderSpec->Init(_proxyArchive, &_proxyFolderItem->Folders[index], 
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
  CMyComPtr<IFolderFolder> parentFolder = _parentFolder;
  *resultFolder = parentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetName(BSTR *name)
{
  CMyComBSTR temp = _proxyFolderItem->Name;
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

STDMETHODIMP CAgentFolder::GetNumberOfProperties(UINT32 *numProperties)
{
  return _agentSpec->_archive->GetNumberOfProperties(numProperties);
}

STDMETHODIMP CAgentFolder::GetPropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  RINOK(_agentSpec->_archive->GetPropertyInfo(index, name, propID, varType));
  if (*propID == kpidPath)
    *propID = kpidName;
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetTypeID(BSTR *name)
{
  // Change It!!!
  UString temp = UString(L"7-zip.") + UString(L"zip");
    //     _agentSpec->_CLSID;
  CMyComBSTR bstrTemp = temp;
  *name = bstrTemp.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetPath(BSTR *path)
{
  UStringVector pathParts;
  pathParts.Clear();
  CMyComPtr<IFolderFolder> currentFolder = this;
  while (true)
  {
    CMyComPtr<IFolderFolder> newFolder;
    currentFolder->BindToParentFolder(&newFolder);  
    if (newFolder == NULL)
      break;
    CMyComBSTR aName;
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

  CMyComBSTR tempPath = prefix;
  *path = tempPath.Detach();
  return S_OK;
}
#endif


STDMETHODIMP CAgentFolder::Extract(const UINT32 *indices, 
    UINT32 numItems, 
    NExtractionMode::NPath::EEnum pathMode, 
    NExtractionMode::NOverwrite::EEnum overwriteMode, 
    const wchar_t *path,
    INT32 testMode,
    IFolderArchiveExtractCallback *extractCallback2)
{
  UINT codePage = GetCurrentFileCodePage();
  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  UStringVector pathParts;
  CProxyFolder *currentProxyFolder = _proxyFolderItem;
  while (currentProxyFolder->Parent)
  {
    pathParts.Insert(0, currentProxyFolder->Name);
    currentProxyFolder = currentProxyFolder->Parent;
  }
  extractCallbackSpec->Init(_agentSpec->_archive, 
      extractCallback2, 
      GetSystemString(path, codePage),
      pathMode, 
      overwriteMode, 
      pathParts, 
      codePage, 
      _agentSpec->DefaultName,
      _agentSpec->DefaultTime, 
      _agentSpec->DefaultAttributes
      // ,_agentSpec->_srcDirectoryPrefix
      );
  CUIntVector realIndices;
  _proxyFolderItem->GetRealIndices(indices, numItems, realIndices);
  return _agentSpec->_archive->Extract(&realIndices.Front(), 
      realIndices.Size(), testMode, extractCallback);
}


/////////////////////////////////////////
// CAgent

CAgent::CAgent():
  _proxyArchive(NULL)
{
}

CAgent::~CAgent()
{
  if (_proxyArchive != NULL)
    delete _proxyArchive;
}

STDMETHODIMP CAgent::Open(
    const wchar_t *filePath, 
    BSTR *archiveType,
    // CLSID *clsIDResult,
    IArchiveOpenCallback *openArchiveCallback)
{
  _archiveFilePath = GetSystemString(filePath, GetCurrentFileCodePage());
  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(_archiveFilePath, fileInfo))
    return ::GetLastError();
  if (fileInfo.IsDirectory())
    return E_FAIL;
  CArchiverInfo archiverInfo;
  HRESULT res = OpenArchive(_archiveFilePath, 
      #ifndef EXCLUDE_COM
      &_library, 
      #endif
      &_archive, archiverInfo, openArchiveCallback);
  RINOK(res);
  DefaultName = GetDefaultName(_archiveFilePath, 
      archiverInfo.Extension, archiverInfo.AddExtension);
  DefaultTime = fileInfo.LastWriteTime;
  DefaultAttributes = fileInfo.Attributes;
  if (archiveType != 0)
  {
    CMyComBSTR name = archiverInfo.Name;
    *archiveType = name.Detach();
  }
  return S_OK;
}

STDMETHODIMP CAgent::ReOpen(
    // const wchar_t *filePath, 
    IArchiveOpenCallback *openArchiveCallback)
{
  UINT codePage = GetCurrentFileCodePage();
  if (_proxyArchive != NULL)
  {
    delete _proxyArchive;
    _proxyArchive = NULL;
  }
  RINOK(ReOpenArchive(_archive, _archiveFilePath));
  return ReadItems();
}

STDMETHODIMP CAgent::Close()
{
  return _archive->Close();
}

/*
STDMETHODIMP CAgent::EnumProperties(IEnumSTATPROPSTG **EnumProperties)
{
  return _archive->EnumProperties(EnumProperties);
}
*/

HRESULT CAgent::ReadItems()
{
  if (_proxyArchive != NULL)
    return S_OK;
  _proxyArchive = new CProxyArchive();
  return _proxyArchive->Load(_archive, 
      DefaultName, 
      // _defaultTime, 
      // _defaultAttributes, 
      NULL);
}

STDMETHODIMP CAgent::BindToRootFolder(IFolderFolder **resultFolder)
{
  RINOK(ReadItems());
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> rootFolder = folderSpec;
  folderSpec->Init(_proxyArchive, &_proxyArchive->RootFolder, NULL, this);
  *resultFolder = rootFolder.Detach();
  return S_OK;
}


STDMETHODIMP CAgent::Extract(
    NExtractionMode::NPath::EEnum pathMode, 
    NExtractionMode::NOverwrite::EEnum overwriteMode, 
    const wchar_t *path,
    INT32 testMode,
    IFolderArchiveExtractCallback *extractCallback2)
{
  UINT codePage = GetCurrentFileCodePage();
  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  extractCallbackSpec->Init(_archive, 
      extractCallback2, 
      GetSystemString(path, codePage),
      pathMode, 
      overwriteMode, 
      UStringVector(), 
      codePage, 
      DefaultName,
      DefaultTime, 
      DefaultAttributes
      // ,_srcDirectoryPrefix
      );
  return _archive->Extract(0, -1, testMode, extractCallback);
}

STDMETHODIMP CAgent::GetNumberOfProperties(UINT32 *numProperties)
{
  return _archive->GetNumberOfProperties(numProperties);
}

STDMETHODIMP CAgent::GetPropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  RINOK(_archive->GetPropertyInfo(index, name, propID, varType));
  if (*propID == kpidPath)
    *propID = kpidName;
  return S_OK;
}

STDMETHODIMP CAgent::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  return _archive->GetArchiveProperty(propID, value);
}

STDMETHODIMP CAgent::GetNumberOfArchiveProperties(UINT32 *numProperties)
{
  return _archive->GetNumberOfArchiveProperties(numProperties);
}

STDMETHODIMP CAgent::GetArchivePropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return _archive->GetArchivePropertyInfo(index,     
      name, propID, varType);
}

