// FolderOut.cpp

#include "StdAfx.h"
#include "Agent.h"

#include "Common/StringConvert.h"
#include "Windows/FileDir.h"

// #include "../Common/CompressEngineCommon.h"
#include "../Common/ZipRegistry.h"
#include "../Common/UpdateAction.h"
#include "../Common/WorkDir.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

static LPCTSTR kTempArcivePrefix = TEXT("7zi");

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

void CAgentFolder::GetPathParts(UStringVector &pathParts)
{
  pathParts.Clear();
  CMyComPtr<IFolderFolder> folder = this;
  while (true)
  {
    CMyComPtr<IFolderFolder> newFolder;
    folder->BindToParentFolder(&newFolder);  
    if (newFolder == NULL)
      break;
    CMyComBSTR name;
    folder->GetName(&name);
    pathParts.Insert(0, (const wchar_t *)name);
    folder = newFolder;
  }
}

HRESULT CAgentFolder::CommonUpdateOperation(
    bool deleteOperation,
    bool createFolderOperation,
    bool renameOperation,
    const wchar_t *newItemName, 
    const NUpdateArchive::CActionSet *actionSet,
    const UINT32 *indices, UINT32 numItems,
    IFolderArchiveUpdateCallback *updateCallback100)
{
  UINT codePage = GetCurrentFileCodePage();
  // CZipRegistryManager aZipRegistryManager;
  NWorkDir::CInfo workDirInfo;
  ReadWorkDirInfo(workDirInfo);
  CSysString archiveFilePath  = _agentSpec->_archiveFilePath;
  CSysString workDir = GetWorkDir(workDirInfo, archiveFilePath );
  CreateComplexDirectory(workDir);

  CTempFile tempFile;
  CSysString tempFileName;
  if (tempFile.Create(workDir, kTempArcivePrefix, tempFileName) == 0)
    return E_FAIL;



  /*
  if (SetOutProperties(anOutArchive, aCompressionInfo.Method) != S_OK)
    return NFileOperationReturnCode::kError;
  */
  
  ////////////////////////////
  // Save FolderItem;

  UStringVector pathParts;
  GetPathParts(pathParts);

  HRESULT result;
  if (deleteOperation)
    result = _agentSpec->DeleteItems(GetUnicodeString(tempFileName, codePage),
        indices, numItems, updateCallback100);
  else if (createFolderOperation)
  {
    result = _agentSpec->CreateFolder(GetUnicodeString(tempFileName, codePage),
        newItemName, updateCallback100);
  }
  else if (renameOperation)
  {
    result = _agentSpec->RenameItem(
        GetUnicodeString(tempFileName, codePage),
        indices, numItems, 
        newItemName, 
        updateCallback100);
  }
  else
  {
    BYTE actionSetByte[NUpdateArchive::NPairState::kNumValues];
    for (int i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
      actionSetByte[i] = actionSet->StateActions[i];
    result = _agentSpec->DoOperation(NULL, NULL,
        GetUnicodeString(tempFileName, codePage),
        actionSetByte, NULL, updateCallback100);
  }
  
  if (result != S_OK)
    return result;

  _agentSpec->Close();
  
  // m_FolderItem = NULL;
  
  if (!DeleteFileAlways(archiveFilePath ))
    return GetLastError();

  tempFile.DisableDeleting();
  if (!MoveFile(tempFileName, archiveFilePath ))
    return GetLastError();
  
  RINOK(_agentSpec->ReOpen(NULL));
   
  ////////////////////////////
  // Restore FolderItem;

  CMyComPtr<IFolderFolder> archiveFolder;
  RINOK(_agentSpec->BindToRootFolder(&archiveFolder));
  for (int i = 0; i < pathParts.Size(); i++)
  {
    CMyComPtr<IFolderFolder> newFolder;
    archiveFolder->BindToFolder(pathParts[i], &newFolder);
    if(!newFolder)
      break;
    archiveFolder = newFolder;
  }

  CMyComPtr<IArchiveFolderInternal> archiveFolderInternal;
  RINOK(archiveFolder.QueryInterface(IID_IArchiveFolderInternal, &archiveFolderInternal));
  CAgentFolder *agentFolder;
  RINOK(archiveFolderInternal->GetAgentFolder(&agentFolder));
  _proxyFolderItem = agentFolder->_proxyFolderItem;
  _proxyArchive = agentFolder->_proxyArchive;
  _parentFolder = agentFolder->_parentFolder;

  return S_OK;
}

STDMETHODIMP CAgentFolder::CopyFrom(
    const wchar_t *fromFolderPath, // test it
    const wchar_t **itemsPaths,
    UINT32 numItems,
    IProgress *progress)
{
  RINOK(_agentSpec->SetFiles(fromFolderPath, itemsPaths, numItems));
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress != 0)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(
      IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(false, false, false, NULL, 
    &NUpdateArchive::kAddActionSet, 0, 0, updateCallback100);
}

STDMETHODIMP CAgentFolder::Delete(const UINT32 *indices, UINT32 numItems, IProgress *progress)
{
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress != 0)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(
        IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(true, false, false, NULL, 
    &NUpdateArchive::kDeleteActionSet, indices, numItems, updateCallback100);
}

STDMETHODIMP CAgentFolder::CreateFolder(const wchar_t *name, IProgress *progress)
{
  if (_proxyFolderItem->FindDirSubItemIndex(name) >= 0)
    return ERROR_ALREADY_EXISTS;
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress != 0)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(
        IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(false, true, false, name, NULL, NULL, 
      0, updateCallback100);
}

STDMETHODIMP CAgentFolder::Rename(UINT32 index, const wchar_t *newName, IProgress *progress)
{
  CUIntVector realIndices;
  CUIntVector indices;
  indices.Add(index);
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress != 0)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(
        IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(false, false, true, newName, NULL, &indices.Front(), 
      indices.Size(), updateCallback100);
  return E_NOTIMPL;
}

STDMETHODIMP CAgentFolder::CreateFile(const wchar_t *name, IProgress *progress)
{
  return E_NOTIMPL;
}

STDMETHODIMP CAgentFolder::SetProperty(UINT32 index, PROPID propID, 
    const PROPVARIANT *value, IProgress *progress)
{
  return E_NOTIMPL;
}
