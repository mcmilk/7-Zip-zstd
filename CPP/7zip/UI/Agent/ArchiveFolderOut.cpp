// FolderOut.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"

#include "Windows/FileDir.h"

#include "../Common/WorkDir.h"

#include "Agent.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

static LPCWSTR kTempArcivePrefix = L"7zA";

void CAgentFolder::GetPathParts(UStringVector &pathParts)
{
  _proxyFolderItem->GetPathParts(pathParts);
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
  NWorkDir::CInfo workDirInfo;
  workDirInfo.Load();
  UString archiveFilePath  = _agentSpec->_archiveFilePath;
  UString workDir = GetWorkDir(workDirInfo, archiveFilePath);
  CreateComplexDirectory(workDir);

  CTempFileW tempFile;
  UString tempFileName;
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
    result = _agentSpec->DeleteItems(tempFileName,
        indices, numItems, updateCallback100);
  else if (createFolderOperation)
  {
    result = _agentSpec->CreateFolder(tempFileName,
        newItemName, updateCallback100);
  }
  else if (renameOperation)
  {
    result = _agentSpec->RenameItem(
        tempFileName,
        indices, numItems,
        newItemName,
        updateCallback100);
  }
  else
  {
    Byte actionSetByte[NUpdateArchive::NPairState::kNumValues];
    for (int i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
      actionSetByte[i] = (Byte)actionSet->StateActions[i];
    result = _agentSpec->DoOperation2(tempFileName, actionSetByte, NULL, updateCallback100);
  }
  
  if (result != S_OK)
    return result;

  _agentSpec->Close();
  
  // m_FolderItem = NULL;
  
  if (NFind::DoesFileExist(archiveFilePath))
    if (!DeleteFileAlways(archiveFilePath))
      return GetLastError();

  tempFile.DisableDeleting();
  if (!MyMoveFile(tempFileName, archiveFilePath))
    return GetLastError();

  {
    CMyComPtr<IArchiveOpenCallback> openCallback;
    if (updateCallback100)
    {
      RINOK(updateCallback100->QueryInterface(IID_IArchiveOpenCallback, (void **)&openCallback));
    }
    RINOK(_agentSpec->ReOpen(openCallback));
  }
   
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
  COM_TRY_BEGIN
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress != 0)
  {
    RINOK(progress->QueryInterface(IID_IFolderArchiveUpdateCallback, (void **)&updateCallback100));
  }
  try
  {
    RINOK(_agentSpec->SetFiles(fromFolderPath, itemsPaths, numItems));
    RINOK(_agentSpec->SetFolder(this));
    return CommonUpdateOperation(false, false, false, NULL,
      &NUpdateArchive::kAddActionSet, 0, 0, updateCallback100);
  }
  catch(const UString &s)
  {
    RINOK(updateCallback100->UpdateErrorMessage(UString(L"Error: ") + s));
    return E_FAIL;
  }
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::Delete(const UINT32 *indices, UINT32 numItems, IProgress *progress)
{
  COM_TRY_BEGIN
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
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::CreateFolder(const wchar_t *name, IProgress *progress)
{
  COM_TRY_BEGIN
  if (_proxyFolderItem->FindDirSubItemIndex(name) >= 0)
    return ERROR_ALREADY_EXISTS;
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress != 0)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(false, true, false, name, NULL, NULL, 0, updateCallback100);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::Rename(UINT32 index, const wchar_t *newName, IProgress *progress)
{
  COM_TRY_BEGIN
  CUIntVector indices;
  indices.Add(index);
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress != 0)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(false, false, true, newName, NULL, &indices.Front(),
      indices.Size(), updateCallback100);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::CreateFile(const wchar_t * /* name */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CAgentFolder::SetProperty(UINT32 /* index */, PROPID /* propID */,
    const PROPVARIANT * /* value */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}
