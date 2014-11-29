// PluginDelete.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "Messages.h"
#include "Plugin.h"
#include "UpdateCallbackFar.h"

using namespace NFar;

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
      sprintf(msg, g_StartupInfo.GetMsgString(NMessageID::kDeleteNumberOfFiles), numItems);
      msgItems[1] = msg;
    }
    if (g_StartupInfo.ShowMessage(FMSG_WARNING, NULL, msgItems, ARRAY_SIZE(msgItems), 2) != 0)
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

  CWorkDirTempFile tempFile;
  if (tempFile.CreateTempFile(m_FileName) != S_OK)
    return FALSE;

  CObjArray<UInt32> indices(numItems);
  int i;
  for (i = 0; i < numItems; i++)
    indices[i] = (UInt32)panelItems[i].UserData;

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
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback(updateCallbackSpec);
  
  updateCallbackSpec->Init(/* m_ArchiveHandler, */ progressBoxPointer);

  result = outArchive->DeleteItems(tempFile.OutStream, indices, numItems, updateCallback);
  updateCallback.Release();
  outArchive.Release();

  if (result == S_OK)
  {
    result = AfterUpdate(tempFile, pathVector);
  }
  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return FALSE;
  }
  SetCurrentDirVar();
  return TRUE;
}
