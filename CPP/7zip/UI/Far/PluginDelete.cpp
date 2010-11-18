// PluginDelete.cpp

#include "StdAfx.h"

#include "Windows/FileDir.h"

#include "../Common/WorkDir.h"

#include "Messages.h"
#include "Plugin.h"
#include "UpdateCallback100.h"

using namespace NFar;
using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

static LPCWSTR kTempArchivePrefix = L"7zA";

int CPlugin::DeleteFiles(PluginPanelItem *panelItems, int numItems, int opMode)
{
  if (numItems == 0)
    return FALSE;
  /*
  if (!m_ArchiverInfo.UpdateEnabled)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return FALSE;
  }
  */
  if ((opMode & OPM_SILENT) == 0)
  {
    const char *msgItems[]=
    {
      g_StartupInfo.GetMsgString(NMessageID::kDeleteTitle),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteFiles),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteDelete),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteCancel)
    };
    char msg[1024];
    if (numItems == 1)
    {
      sprintf(msg, g_StartupInfo.GetMsgString(NMessageID::kDeleteFile), panelItems[0].FindData.cFileName);
      msgItems[1] = msg;
    }
    else if (numItems > 1)
    {
      sprintf(msg, g_StartupInfo.GetMsgString(NMessageID::kDeleteNumberOfFiles),
          numItems);
      msgItems[1] = msg;
    }
    if (g_StartupInfo.ShowMessage(FMSG_WARNING, NULL, msgItems,
        sizeof(msgItems) / sizeof(msgItems[0]), 2) != 0)
      return (FALSE);
  }

  CScreenRestorer screenRestorer;
  CProgressBox progressBox;
  CProgressBox *progressBoxPointer = NULL;
  if ((opMode & OPM_SILENT) == 0 && (opMode & OPM_FIND ) == 0)
  {
    screenRestorer.Save();

    progressBoxPointer = &progressBox;
    progressBox.Init(
        // g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kDeleting), 48);
  }

  NWorkDir::CInfo workDirInfo;
  workDirInfo.Load();

  UString workDir = GetWorkDir(workDirInfo, m_FileName);
  CreateComplexDirectory(workDir);

  CTempFileW tempFile;
  UString tempFileName;
  if (tempFile.Create(workDir, kTempArchivePrefix, tempFileName) == 0)
    return FALSE;


  CRecordVector<UINT32> indices;
  indices.Reserve(numItems);
  int i;
  for (i = 0; i < numItems; i++)
    indices.Add((UINT32)panelItems[i].UserData);

  ////////////////////////////
  // Save _folder;

  UStringVector pathVector;
  GetPathParts(pathVector);
  
  CMyComPtr<IOutFolderArchive> outArchive;
  HRESULT result = m_ArchiveHandler.QueryInterface(IID_IOutFolderArchive, &outArchive);
  if (result != S_OK)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return FALSE;
  }
  outArchive->SetFolder(_folder);

  CUpdateCallback100Imp *updateCallbackSpec = new CUpdateCallback100Imp;
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback(updateCallbackSpec );
  
  updateCallbackSpec->Init(/* m_ArchiveHandler, */ progressBoxPointer);


  result = outArchive->DeleteItems(
      tempFileName,
      &indices.Front(), indices.Size(),
      updateCallback);
  updateCallback.Release();
  outArchive.Release();

  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return FALSE;
  }

  _folder.Release();
  m_ArchiveHandler->Close();
  
  if (!DeleteFileAlways(m_FileName))
  {
    ShowLastErrorMessage();
    return FALSE;
  }

  tempFile.DisableDeleting();
  if (!MyMoveFile(tempFileName, m_FileName))
  {
    ShowLastErrorMessage();
    return FALSE;
  }
  
  result = m_ArchiveHandler->ReOpen(NULL);
  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return FALSE;
  }

 
  ////////////////////////////
  // Restore _folder;

  m_ArchiveHandler->BindToRootFolder(&_folder);
  for (i = 0; i < pathVector.Size(); i++)
  {
    CMyComPtr<IFolderFolder> newFolder;
    _folder->BindToFolder(pathVector[i], &newFolder);
    if (!newFolder)
      break;
    _folder = newFolder;
  }
  GetCurrentDir();

  return TRUE;
}
