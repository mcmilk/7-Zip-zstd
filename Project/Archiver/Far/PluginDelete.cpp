// PluginDelete.cpp

#include "StdAfx.h"

#include "Plugin.h"
#include "Messages.h"
#include "UpdateCallback100.h"

#include "Windows/FileDir.h"

#include "Interface/FileStreams.h"

#include "Common/StringConvert.h"

#include "../Common/ZipRegistry.h"
#include "../Common/UpdateUtils.h"
#include "../Common/OpenEngine2.h"


using namespace std;
using namespace NFar;
using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

static LPCTSTR kTempArcivePrefix = "7zi";

int CPlugin::DeleteFiles(PluginPanelItem *panelItems, int numItems,
    int opMode)
{
  if (numItems == 0)
    return FALSE;
  if (!m_ArchiverInfo.UpdateEnabled)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return FALSE;
  }
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
      sprintf(msg, g_StartupInfo.GetMsgString(NMessageID::kDeleteFile),
          panelItems[0].FindData.cFileName);
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
    progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kDeleting), 1 << 17);
  }

  NZipSettings::NWorkDir::CInfo workDirInfo;
  NZipRegistryManager::ReadWorkDirInfo(workDirInfo);
  CSysString workDir = GetWorkDir(workDirInfo, m_FileName);
  CreateComplexDirectory(workDir);

  CTempFile tempFile;
  CSysString tempFileName;
  if (tempFile.Create(workDir, kTempArcivePrefix, tempFileName) == 0)
    return FALSE;


  CRecordVector<UINT32> indices;
  indices.Reserve(numItems);
  for(int i = 0; i < numItems; i++)
    indices.Add(panelItems[i].UserData);

  ////////////////////////////
  // Save _folder;

  UStringVector pathVector;
  GetPathParts(pathVector);
  
  CComPtr<IOutFolderArchive> outArchive;
  HRESULT result = m_ArchiveHandler.QueryInterface(&outArchive);
  if(result != S_OK)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return FALSE;
  }
  outArchive->SetFolder(_folder);

  CComObjectNoLock<CUpdateCallBack100Imp> *updateCallbackSpec =
    new CComObjectNoLock<CUpdateCallBack100Imp>;
  CComPtr<IFolderArchiveUpdateCallback> updateCallback(updateCallbackSpec );
  
  updateCallbackSpec->Init(m_ArchiveHandler, &progressBox);


  result = outArchive->DeleteItems(
      MultiByteToUnicodeString(tempFileName, CP_OEMCP), 
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
  if (!MoveFile(tempFileName, m_FileName))
  {
    ShowLastErrorMessage();
    return FALSE;
  }
  
  result = ReOpenArchive(m_ArchiveHandler, m_DefaultName, m_FileName);
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
    CComPtr<IFolderFolder> newFolder;
    _folder->BindToFolder(pathVector[i], &newFolder);
    if(!newFolder  )
      break;
    _folder = newFolder;
  }

  return(TRUE);
}
