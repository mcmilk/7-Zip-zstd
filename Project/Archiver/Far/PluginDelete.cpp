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

int CPlugin::DeleteFiles(PluginPanelItem *aPanelItems, int anItemsNumber,
    int anOpMode)
{
  if (anItemsNumber == 0)
    return FALSE;
  if (!m_ArchiverInfo.UpdateEnabled)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return FALSE;
  }
  if ((anOpMode & OPM_SILENT) == 0)
  {
    const char *aMsgItems[]=
    {
      g_StartupInfo.GetMsgString(NMessageID::kDeleteTitle),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteFiles),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteDelete),
      g_StartupInfo.GetMsgString(NMessageID::kDeleteCancel)
    };
    char aMsg[1024];
    if (anItemsNumber == 1)
    {
      sprintf(aMsg, g_StartupInfo.GetMsgString(NMessageID::kDeleteFile),
          aPanelItems[0].FindData.cFileName);
      aMsgItems[1] = aMsg;
    }
    else if (anItemsNumber > 1)
    {
      sprintf(aMsg, g_StartupInfo.GetMsgString(NMessageID::kDeleteNumberOfFiles), 
          anItemsNumber);
      aMsgItems[1] = aMsg;
    }
    if (g_StartupInfo.ShowMessage(FMSG_WARNING, NULL, aMsgItems, 
        sizeof(aMsgItems) / sizeof(aMsgItems[0]), 2) != 0)
      return (FALSE);
  }

  CScreenRestorer aScreenRestorer;
  CProgressBox aProgressBox;
  CProgressBox *aProgressBoxPointer = NULL;
  if ((anOpMode & OPM_SILENT) == 0 && (anOpMode & OPM_FIND ) == 0)
  {
    aScreenRestorer.Save();

    aProgressBoxPointer = &aProgressBox;
    aProgressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kDeleting), 1 << 17);
  }

  CZipRegistryManager aZipRegistryManager;
  NZipSettings::NWorkDir::CInfo aWorkDirInfo;
  aZipRegistryManager.ReadWorkDirInfo(aWorkDirInfo);
  CSysString aWorkDir = GetWorkDir(aWorkDirInfo, m_FileName);
  CreateComplexDirectory(aWorkDir);

  CTempFile aTempFile;
  CSysString aTempFileName;
  if (aTempFile.Create(aWorkDir, kTempArcivePrefix, aTempFileName) == 0)
    return FALSE;


  CRecordVector<UINT32> anIndexes;
  anIndexes.Reserve(anItemsNumber);
  for(int i = 0; i < anItemsNumber; i++)
    anIndexes.Add(aPanelItems[i].UserData);

  ////////////////////////////
  // Save m_ArchiveFolder;

  UStringVector aPathVector;
  GetPathParts(aPathVector);
  
  CComPtr<IOutArchiveHandler100> anOutArchive;
  HRESULT aResult = m_ArchiveHandler.QueryInterface(&anOutArchive);
  if(aResult != S_OK)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return FALSE;
  }
  anOutArchive->SetFolder(m_ArchiveFolder);

  CComObjectNoLock<CUpdateCallBack100Imp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBack100Imp>;
  CComPtr<IUpdateCallback100> anUpdateCallBack(anUpdateCallBackSpec );
  
  anUpdateCallBackSpec->Init(m_ArchiveHandler, &aProgressBox);


  aResult = anOutArchive->DeleteItems(
      MultiByteToUnicodeString(aTempFileName, CP_OEMCP), 
      &anIndexes.Front(), anIndexes.Size(),
      anUpdateCallBack);
  anUpdateCallBack.Release();
  anOutArchive.Release();

  if (aResult != S_OK)
  {
    ShowErrorMessage(aResult);
    return FALSE;
  }

  m_ArchiveFolder.Release();
  m_ArchiveHandler->Close();
  
  if (!DeleteFileAlways(m_FileName))
  {
    ShowLastErrorMessage();
    return FALSE;
  }

  aTempFile.DisableDeleting();
  if (!MoveFile(aTempFileName, m_FileName))
  {
    ShowLastErrorMessage();
    return FALSE;
  }
  
  aResult = ReOpenArchive(m_ArchiveHandler, m_DefaultName, m_FileName);
  if (aResult != S_OK)
  {
    ShowErrorMessage(aResult);
    return FALSE;
  }

 
  ////////////////////////////
  // Restore m_ArchiveFolder;

  m_ArchiveHandler->BindToRootFolder(&m_ArchiveFolder);
  for (i = 0; i < aPathVector.Size(); i++)
  {
    CComPtr<IArchiveFolder> aNewFolder;
    m_ArchiveFolder->BindToFolder(aPathVector[i], &aNewFolder);
    if(!aNewFolder  )
      break;
    m_ArchiveFolder = aNewFolder;
  }

  return(TRUE);
}
