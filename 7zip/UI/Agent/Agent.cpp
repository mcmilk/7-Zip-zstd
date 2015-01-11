// Agent.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileFind.h"

#include "../Common/OpenArchive.h"
#include "../Common/DefaultName.h"

#include "Agent.h"
#include "ArchiveExtractCallback.h"

#ifdef FORMAT_7Z
#include "../../Archive/7z/7zHandler.h"
#endif

using namespace NWindows;

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
  COM_TRY_BEGIN
  if (index >= (UINT32)_proxyFolderItem->Folders.Size())
    return E_INVALIDARG;
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> agentFolder = folderSpec;
  folderSpec->Init(_proxyArchive, &_proxyFolderItem->Folders[index], 
      this, _agentSpec);
  *resultFolder = agentFolder.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  int index = _proxyFolderItem->FindDirSubItemIndex(name);
  if (index < 0)
    return E_INVALIDARG;
  return BindToFolder(index, resultFolder);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  CMyComPtr<IFolderFolder> parentFolder = _parentFolder;
  *resultFolder = parentFolder.Detach();
  return S_OK;
  COM_TRY_END
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
  COM_TRY_BEGIN
  return _agentSpec->_archive->GetNumberOfProperties(numProperties);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetPropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  RINOK(_agentSpec->_archive->GetPropertyInfo(index, name, propID, varType));
  if (*propID == kpidPath)
    *propID = kpidName;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetTypeID(BSTR *name)
{
  COM_TRY_BEGIN
  UString temp = UString(L"7-Zip.") + _agentSpec->ArchiveType;
  CMyComBSTR bstrTemp = temp;
  *name = bstrTemp.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetPath(BSTR *path)
{
  COM_TRY_BEGIN
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
  COM_TRY_END
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
  COM_TRY_BEGIN
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
      path,
      pathMode, 
      overwriteMode, 
      pathParts, 
      _agentSpec->DefaultName,
      _agentSpec->DefaultTime, 
      _agentSpec->DefaultAttributes
      // ,_agentSpec->_srcDirectoryPrefix
      );
  CUIntVector realIndices;
  _proxyFolderItem->GetRealIndices(indices, numItems, realIndices);
  return _agentSpec->_archive->Extract(&realIndices.Front(), 
      realIndices.Size(), testMode, extractCallback);
  COM_TRY_END
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
  COM_TRY_BEGIN
  _archiveFilePath = filePath;
  NFile::NFind::CFileInfoW fileInfo;
  if (!NFile::NFind::FindFile(_archiveFilePath, fileInfo))
    return ::GetLastError();
  if (fileInfo.IsDirectory())
    return E_FAIL;
  CArchiverInfo archiverInfo;
  int subExtIndex;
  HRESULT res = OpenArchive(_archiveFilePath, 
      #ifndef EXCLUDE_COM
      &_library, 
      #endif
      &_archive, archiverInfo, subExtIndex, openArchiveCallback);
  RINOK(res);
  DefaultName = GetDefaultName(_archiveFilePath, 
      archiverInfo.Extensions[subExtIndex].Extension, 
      archiverInfo.Extensions[subExtIndex].AddExtension);
  DefaultTime = fileInfo.LastWriteTime;
  DefaultAttributes = fileInfo.Attributes;
  ArchiveType = archiverInfo.Name;
  if (archiveType != 0)
  {
    CMyComBSTR name = archiverInfo.Name;
    *archiveType = name.Detach();
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgent::ReOpen(
    // const wchar_t *filePath, 
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  if (_proxyArchive != NULL)
  {
    delete _proxyArchive;
    _proxyArchive = NULL;
  }
  RINOK(ReOpenArchive(_archive, _archiveFilePath));
  return ReadItems();
  COM_TRY_END
}

STDMETHODIMP CAgent::Close()
{
  COM_TRY_BEGIN
  return _archive->Close();
  COM_TRY_END
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
  COM_TRY_BEGIN
  RINOK(ReadItems());
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> rootFolder = folderSpec;
  folderSpec->Init(_proxyArchive, &_proxyArchive->RootFolder, NULL, this);
  *resultFolder = rootFolder.Detach();
  return S_OK;
  COM_TRY_END
}


STDMETHODIMP CAgent::Extract(
    NExtractionMode::NPath::EEnum pathMode, 
    NExtractionMode::NOverwrite::EEnum overwriteMode, 
    const wchar_t *path,
    INT32 testMode,
    IFolderArchiveExtractCallback *extractCallback2)
{
  COM_TRY_BEGIN
  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  extractCallbackSpec->Init(_archive, 
      extractCallback2, 
      path,
      pathMode, 
      overwriteMode, 
      UStringVector(), 
      DefaultName,
      DefaultTime, 
      DefaultAttributes
      // ,_srcDirectoryPrefix
      );
  return _archive->Extract(0, -1, testMode, extractCallback);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetNumberOfProperties(UINT32 *numProperties)
{
  COM_TRY_BEGIN
  return _archive->GetNumberOfProperties(numProperties);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetPropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  RINOK(_archive->GetPropertyInfo(index, name, propID, varType));
  if (*propID == kpidPath)
    *propID = kpidName;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgent::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  return _archive->GetArchiveProperty(propID, value);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetNumberOfArchiveProperties(UINT32 *numProperties)
{
  COM_TRY_BEGIN
  return _archive->GetNumberOfArchiveProperties(numProperties);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetArchivePropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  return _archive->GetArchivePropertyInfo(index,     
      name, propID, varType);
  COM_TRY_END
}

