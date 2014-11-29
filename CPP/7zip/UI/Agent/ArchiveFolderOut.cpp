// FolderOut.cpp

#include "StdAfx.h"

#include "../../../Common/ComTry.h"

#include "../../../Windows/FileDir.h"

#include "../../Common/FileStreams.h"
#include "../../Common/LimitedStreams.h"

#include "../../Compress/CopyCoder.h"

#include "../Common/WorkDir.h"

#include "Agent.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;

void CAgentFolder::GetPathParts(UStringVector &pathParts)
{
  if (_proxyArchive2)
    _proxyArchive2->GetPathParts(_proxyFolderItem, pathParts);
  else
    _proxyArchive->GetPathParts(_proxyFolderItem, pathParts);
}

static bool DeleteEmptyFolderAndEmptySubFolders(const FString &path)
{
  NFind::CFileInfo fileInfo;
  FString pathPrefix = path + FCHAR_PATH_SEPARATOR;
  {
    NFind::CEnumerator enumerator(pathPrefix + FCHAR_ANY_MASK);
    while (enumerator.Next(fileInfo))
    {
      if (fileInfo.IsDir())
        if (!DeleteEmptyFolderAndEmptySubFolders(pathPrefix + fileInfo.Name))
          return false;
    }
  }
  /*
  // we don't need clear readonly for folders
  if (!SetFileAttrib(path, 0))
    return false;
  */
  return RemoveDir(path);
}


HRESULT CAgentFolder::CommonUpdateOperation(
    AGENT_OP operation,
    bool moveMode,
    const wchar_t *newItemName,
    const NUpdateArchive::CActionSet *actionSet,
    const UInt32 *indices, UInt32 numItems,
    IFolderArchiveUpdateCallback *updateCallback100)
{
  if (!_agentSpec->CanUpdate())
    return E_NOTIMPL;

  ////////////////////////////
  // Save FolderItem;

  UStringVector pathParts;
  GetPathParts(pathParts);

  FStringVector requestedPaths;
  FStringVector processedPaths;

  CWorkDirTempFile tempFile;
  RINOK(tempFile.CreateTempFile(us2fs(_agentSpec->_archiveFilePath)));
  {
    CMyComPtr<IOutStream> tailStream;
    const CArc &arc = *_agentSpec->_archiveLink.GetArc();

    if (arc.ArcStreamOffset == 0)
      tailStream = tempFile.OutStream;
    else
    {
      if (arc.Offset < 0)
        return E_NOTIMPL;
      RINOK(arc.InStream->Seek(0, STREAM_SEEK_SET, NULL));
      RINOK(NCompress::CopyStream_ExactSize(arc.InStream, tempFile.OutStream, arc.ArcStreamOffset, NULL));
      CTailOutStream *tailStreamSpec = new CTailOutStream;
      tailStream = tailStreamSpec;
      tailStreamSpec->Stream = tempFile.OutStream;
      tailStreamSpec->Offset = arc.ArcStreamOffset;
      tailStreamSpec->Init();
    }
    
    HRESULT result;

    switch (operation)
    {
      case AGENT_OP_Delete:
        result = _agentSpec->DeleteItems(tailStream, indices, numItems, updateCallback100);
        break;
      case AGENT_OP_CreateFolder:
        result = _agentSpec->CreateFolder(tailStream, newItemName, updateCallback100);
        break;
      case AGENT_OP_Rename:
        result = _agentSpec->RenameItem(tailStream, indices, numItems, newItemName, updateCallback100);
        break;
      case AGENT_OP_CopyFromFile:
        result = _agentSpec->UpdateOneFile(tailStream, indices, numItems, newItemName, updateCallback100);
        break;
      case AGENT_OP_Uni:
        {
          Byte actionSetByte[NUpdateArchive::NPairState::kNumValues];
          for (int i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
            actionSetByte[i] = (Byte)actionSet->StateActions[i];
          result = _agentSpec->DoOperation2(
              moveMode ? &requestedPaths : NULL,
              moveMode ? &processedPaths : NULL,
              tailStream, actionSetByte, NULL, updateCallback100);
          break;
        }
      default:
        return E_FAIL;
    }
    
    RINOK(result);
  }

  _agentSpec->KeepModeForNextOpen();
  _agentSpec->Close();
  
  // before 9.26: if there was error for MoveToOriginal archive was closed.
  // now: we reopen archive after close

  // m_FolderItem = NULL;
  
  HRESULT res = tempFile.MoveToOriginal(true);

  // RINOK(res);
  if (res == S_OK)
  {
    if (moveMode)
    {
      unsigned i;
      for (i = 0; i < processedPaths.Size(); i++)
      {
        DeleteFileAlways(processedPaths[i]);
      }
      for (i = 0; i < requestedPaths.Size(); i++)
      {
        const FString &fs = requestedPaths[i];
        if (NFind::DoesDirExist(fs))
          DeleteEmptyFolderAndEmptySubFolders(fs);
      }
    }
  }


  {
    CMyComPtr<IArchiveOpenCallback> openCallback;
    if (updateCallback100)
    {
      RINOK(updateCallback100->QueryInterface(IID_IArchiveOpenCallback, (void **)&openCallback));
    }
    RINOK(_agentSpec->ReOpen(openCallback));
  }
   
  // Restore FolderItem;

  CMyComPtr<IFolderFolder> archiveFolder;
  RINOK(_agentSpec->BindToRootFolder(&archiveFolder));
  FOR_VECTOR (i, pathParts)
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
  _proxyArchive2 = agentFolder->_proxyArchive2;
  _parentFolder = agentFolder->_parentFolder;

  return res;
}

STDMETHODIMP CAgentFolder::CopyFrom(Int32 moveMode,
    const wchar_t *fromFolderPath, // test it
    const wchar_t **itemsPaths,
    UInt32 numItems,
    IProgress *progress)
{
  COM_TRY_BEGIN
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress)
  {
    RINOK(progress->QueryInterface(IID_IFolderArchiveUpdateCallback, (void **)&updateCallback100));
  }
  try
  {
    RINOK(_agentSpec->SetFiles(fromFolderPath, itemsPaths, numItems));
    RINOK(_agentSpec->SetFolder(this));
    return CommonUpdateOperation(AGENT_OP_Uni, (moveMode != 0), NULL,
        &NUpdateArchive::k_ActionSet_Add,
        0, 0, updateCallback100);
  }
  catch(const UString &s)
  {
    RINOK(updateCallback100->UpdateErrorMessage(UString(L"Error: ") + s));
    return E_FAIL;
  }
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::CopyFromFile(UInt32 destIndex, const wchar_t *itemPath, IProgress * progress)
{
  COM_TRY_BEGIN
  CUIntVector indices;
  indices.Add(destIndex);
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress)
  {
    RINOK(progress->QueryInterface(IID_IFolderArchiveUpdateCallback, (void **)&updateCallback100));
  }
  try
  {
    RINOK(_agentSpec->SetFolder(this));
    return CommonUpdateOperation(AGENT_OP_CopyFromFile, false, itemPath,
        &NUpdateArchive::k_ActionSet_Add,
        &indices.Front(), indices.Size(), updateCallback100);
  }
  catch(const UString &s)
  {
    RINOK(updateCallback100->UpdateErrorMessage(UString(L"Error: ") + s));
    return E_FAIL;
  }
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::Delete(const UInt32 *indices, UInt32 numItems, IProgress *progress)
{
  COM_TRY_BEGIN
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(
        IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(AGENT_OP_Delete, false, NULL,
    &NUpdateArchive::k_ActionSet_Delete, indices, numItems, updateCallback100);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::CreateFolder(const wchar_t *name, IProgress *progress)
{
  COM_TRY_BEGIN
  if (_proxyArchive2)
  {
    if (_proxyArchive2->IsThere_SubDir(_proxyFolderItem, name))
      return ERROR_ALREADY_EXISTS;
  }
  else
  {
    if (_proxyArchive->FindDirSubItemIndex(_proxyFolderItem, name) >= 0)
      return ERROR_ALREADY_EXISTS;
  }
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(AGENT_OP_CreateFolder, false, name, NULL, NULL, 0, updateCallback100);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::Rename(UInt32 index, const wchar_t *newName, IProgress *progress)
{
  COM_TRY_BEGIN
  CUIntVector indices;
  indices.Add(index);
  RINOK(_agentSpec->SetFolder(this));
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback100;
  if (progress)
  {
    CMyComPtr<IProgress> progressWrapper = progress;
    RINOK(progressWrapper.QueryInterface(IID_IFolderArchiveUpdateCallback, &updateCallback100));
  }
  return CommonUpdateOperation(AGENT_OP_Rename, false, newName, NULL, &indices.Front(),
      indices.Size(), updateCallback100);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::CreateFile(const wchar_t * /* name */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CAgentFolder::SetProperty(UInt32 /* index */, PROPID /* propID */,
    const PROPVARIANT * /* value */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}
