// PluginWrite.cpp

#include "StdAfx.h"

#include "Plugin.h"

#include "Messages.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "../../Archiver/Common/ZipRegistry.h"
#include "../../Archiver/Common/UpdatePairBasic.h"
#include "../../Archiver/Common/CompressEngineCommon.h"
#include "../../Archiver/Common/UpdateUtils.h"

#include "../Common/OpenEngine2.h"

#include "Far/ProgressBox.h"

#include "UpdateCallback100.h"

#include "../Agent/Handler.h"

/*
#include "../../Archiver/Common/DefaultName.h"
#include "../../Archiver/Common/OpenEngine2.h"
*/

// #include "CompressEngine.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;
using namespace NFar;

using namespace NUpdateArchive;

static const char *kHelpTopic =  "Update";

static LPCTSTR kTempArcivePrefix = "7zi";

static const char *kArchiveHistoryKeyName = "7-ZipArcName"; 

static HRESULT SetOutProperties(IOutArchiveHandler100 * anOutArchive, UINT32 aMethod)
{
  CComPtr<ISetProperties> aSetProperties;
  if (anOutArchive->QueryInterface(&aSetProperties) == S_OK)
  {
    CComBSTR aComBSTR;
    switch(aMethod)
    {
      case 0:
        aComBSTR = "0";
        break;
      case 1:
        aComBSTR = "1";
        break;
      case 2:
        aComBSTR = "X";
        break;
      default:
        return E_INVALIDARG;
    }
    CObjectVector<CComBSTR> aNamesReal;
    std::vector<NCOM::CPropVariant> aValues;
    aNamesReal.Add(aComBSTR);
    aValues.push_back(NCOM::CPropVariant());
    std::vector<BSTR> aNames;
    for(int i = 0; i < aNamesReal.Size(); i++)
      aNames.push_back(aNamesReal[i]);
    RETURN_IF_NOT_S_OK(aSetProperties->SetProperties(&aNames.front(), 
      &aValues.front(), aNames.size()));
  }
  return S_OK;
}

NFileOperationReturnCode::EEnum CPlugin::PutFiles(struct PluginPanelItem *aPanelItems, int anItemsNumber,
    int aMove, int anOpMode)
{
  if(aMove != 0)
  {
    g_StartupInfo.ShowMessage(NMessageID::kMoveIsNotSupported);
    return NFileOperationReturnCode::kError;
  }
  if (anItemsNumber == 0)
    return NFileOperationReturnCode::kError;

  if (!m_ArchiverInfo.UpdateEnabled)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return NFileOperationReturnCode::kError;
  }

  static const kYSize = 12;
  static const kXMid = 38;

  NZipSettings::NCompression::CInfo aCompressionInfo;

  CZipRegistryManager aZipRegistryManager;
  aZipRegistryManager.ReadCompressionInfo(aCompressionInfo);
  
  if (!aCompressionInfo.MethodDefined)
    aCompressionInfo.Method = 1;

  const kMethodRadioIndex = 2;
  const kModeRadioIndex = kMethodRadioIndex + 4;

  struct CInitDialogItem anInitItems[]={
    { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kUpdateTitle, NULL, NULL },
    { DI_SINGLEBOX, 4, 2, kXMid - 2, 2 + 4, false, false, 0, false, NMessageID::kUpdateMethod, NULL, NULL },
    { DI_RADIOBUTTON, 6, 3, 0, 0, aCompressionInfo.Method == 0, 
        aCompressionInfo.Method == 0, 
        DIF_GROUP, false, NMessageID::kUpdateMethodStore, NULL, NULL },
    { DI_RADIOBUTTON, 6, 4, 0, 0, aCompressionInfo.Method == 1, 
        aCompressionInfo.Method == 1, 
        0, false, NMessageID::kUpdateMethodNormal, NULL, NULL },
    { DI_RADIOBUTTON, 6, 5, 0, 0, aCompressionInfo.Method == 2,
        aCompressionInfo.Method == 2, 
    false, 0, NMessageID::kUpdateMethodMaximum, NULL, NULL },
    
    { DI_SINGLEBOX, kXMid, 2, 70, 2 + 5, false, false, 0, false, NMessageID::kUpdateMode, NULL, NULL },
    { DI_RADIOBUTTON, kXMid + 2, 3, 0, 0, false, true,
        DIF_GROUP, false, NMessageID::kUpdateModeAdd, NULL, NULL },
    { DI_RADIOBUTTON, kXMid + 2, 4, 0, 0, false, false,
        0, false, NMessageID::kUpdateModeUpdate, NULL, NULL },
    { DI_RADIOBUTTON, kXMid + 2, 5, 0, 0, false, false, 
        0, false, NMessageID::kUpdateModeFreshen, NULL, NULL },
    { DI_RADIOBUTTON, kXMid + 2, 6, 0, 0, false, false,
        0, false, NMessageID::kUpdateModeSynchronize, NULL, NULL },
  
    { DI_TEXT, 3, kYSize - 4, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, "", NULL  },  
    
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kUpdateAdd, NULL, NULL  },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL  }
  };
  
  const kNumDialogItems = sizeof(anInitItems) / sizeof(anInitItems[0]);
  const kOkButtonIndex = kNumDialogItems - 2;
  FarDialogItem aDialogItems[kNumDialogItems];
  g_StartupInfo.InitDialogItems(anInitItems, aDialogItems, kNumDialogItems);
  int anAskCode = g_StartupInfo.ShowDialog(76, kYSize, 
      kHelpTopic, aDialogItems, kNumDialogItems);
  if (anAskCode != kOkButtonIndex)
    return NFileOperationReturnCode::kInterruptedByUser;

  if (aDialogItems[kMethodRadioIndex].Selected)
    aCompressionInfo.SetMethod(0);
  else if (aDialogItems[kMethodRadioIndex + 1].Selected)
    aCompressionInfo.SetMethod(1);
  else if (aDialogItems[kMethodRadioIndex + 2].Selected)
    aCompressionInfo.SetMethod(2);
  else
    throw 51751;

  const CActionSet *anActionSet;

  if (aDialogItems[kModeRadioIndex].Selected)
    anActionSet = &kAddActionSet;
  else if (aDialogItems[kModeRadioIndex + 1].Selected)
    anActionSet = &kUpdateActionSet;
  else if (aDialogItems[kModeRadioIndex + 2].Selected)
      anActionSet = &kFreshActionSet;
  else if (aDialogItems[kModeRadioIndex + 3].Selected)
      anActionSet = &kSynchronizeActionSet;
  else
    throw 51751;

  aZipRegistryManager.SaveCompressionInfo(aCompressionInfo);

  NZipSettings::NWorkDir::CInfo aWorkDirInfo;
  aZipRegistryManager.ReadWorkDirInfo(aWorkDirInfo);
  CSysString aWorkDir = GetWorkDir(aWorkDirInfo, m_FileName);
  CreateComplexDirectory(aWorkDir);

  CTempFile aTempFile;
  CSysString aTempFileName;
  if (aTempFile.Create(aWorkDir, kTempArcivePrefix, aTempFileName) == 0)
    return NFileOperationReturnCode::kError;


  /*
  CSysStringVector aFileNames;
  for(int i = 0; i < anItemsNumber; i++)
  {
    const PluginPanelItem &aPanelItem = aPanelItems[i];
    CSysString aFullName;
    if (!MyGetFullPathName(aPanelItem.FindData.cFileName, aFullName))
      return NFileOperationReturnCode::kError;
    aFileNames.Add(aFullName);
  }
  */

  CScreenRestorer aScreenRestorer;
  CProgressBox aProgressBox;
  CProgressBox *aProgressBoxPointer = NULL;
  if ((anOpMode & OPM_SILENT) == 0 && (anOpMode & OPM_FIND ) == 0)
  {
    aScreenRestorer.Save();

    aProgressBoxPointer = &aProgressBox;
    aProgressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kUpdating), 1 << 16);
  }
 
  ////////////////////////////
  // Save FolderItem;
  UStringVector aPathVector;
  GetPathParts(aPathVector);
  
  /*
  UString anArchivePrefix;
  for(i = aPathVector.Size() - 1; i >= 0; i--)
  {
    anArchivePrefix += aPathVector[i];
    anArchivePrefix += wchar_t(NName::kDirDelimiter);
  }
  /////////////////////////////////
  */

  UStringVector aFileNames;
  aFileNames.Reserve(anItemsNumber);
  for(int i = 0; i < anItemsNumber; i++)
    aFileNames.Add(MultiByteToUnicodeString(aPanelItems[i].FindData.cFileName, CP_OEMCP));
  CRecordVector<const wchar_t *> aFileNamePointers;
  aFileNamePointers.Reserve(anItemsNumber);
  for(i = 0; i < anItemsNumber; i++)
    aFileNamePointers.Add(aFileNames[i]);

  CComPtr<IOutArchiveHandler100> anOutArchive;
  HRESULT aResult = m_ArchiveHandler.QueryInterface(&anOutArchive);
  if(aResult != S_OK)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return NFileOperationReturnCode::kError;
  }
  anOutArchive->SetFolder(m_ArchiveFolder);
  anOutArchive->SetFiles(&aFileNamePointers.Front(), aFileNamePointers.Size());
  m_ArchiveFolder.Release();
  BYTE anActionSetByte[6];
  for (i = 0; i < 6; i++)
    anActionSetByte[i] = anActionSet->StateActions[i];

  CComObjectNoLock<CUpdateCallBack100Imp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBack100Imp>;
  CComPtr<IUpdateCallback100> anUpdateCallBack(anUpdateCallBackSpec );
  
  anUpdateCallBackSpec->Init(m_ArchiveHandler, &aProgressBox);

  if (SetOutProperties(anOutArchive, aCompressionInfo.Method) != S_OK)
    return NFileOperationReturnCode::kError;

  aResult = anOutArchive->DoOperation(NULL,
      MultiByteToUnicodeString(aTempFileName, CP_OEMCP), anActionSetByte, 
      NULL, anUpdateCallBack);
  anUpdateCallBack.Release();
  anOutArchive.Release();

  /*
  HRESULT aResult = Compress(aFileNames, anArchivePrefix, *anActionSet, 
      m_ProxyHandler.get(), 
      m_ArchiverInfo.ClassID, aCompressionInfo.Method == 0,
      aCompressionInfo.Method == 2, aTempFileName, aProgressBoxPointer);
  */

  if (aResult != S_OK)
  {
    ShowErrorMessage(aResult);
    return NFileOperationReturnCode::kError;
  }

  m_ArchiveHandler->Close();
  
  // m_FolderItem = NULL;
  
  if (!DeleteFileAlways(m_FileName))
  {
    ShowLastErrorMessage();
    return NFileOperationReturnCode::kError;
  }

  aTempFile.DisableDeleting();
  if (!MoveFile(aTempFileName, m_FileName))
  {
    ShowLastErrorMessage();
    return NFileOperationReturnCode::kError;
  }
  
  aResult = ReOpenArchive(m_ArchiveHandler, m_DefaultName, m_FileName);
  if (aResult != S_OK)
  {
    ShowErrorMessage(aResult);
    return NFileOperationReturnCode::kError;
  }

  /*
  if(m_ProxyHandler->ReInit(NULL) != S_OK)
    return NFileOperationReturnCode::kError;
  */
  
  ////////////////////////////
  // Restore FolderItem;

  m_ArchiveHandler->BindToRootFolder(&m_ArchiveFolder);
  for (i = 0; i < aPathVector.Size(); i++)
  {
    CComPtr<IArchiveFolder> aNewFolder;
    m_ArchiveFolder->BindToFolder(aPathVector[i], &aNewFolder);
    if(!aNewFolder  )
      break;
    m_ArchiveFolder = aNewFolder;
  }

  /*
  if(aMove != 0)
  {
    for(int i = 0; i < anItemsNumber; i++)
    {
      const PluginPanelItem &aPluginPanelItem = aPanelItems[i];
      bool aResult;
      if(NFile::NFind::NAttributes::IsDirectory(aPluginPanelItem.FindData.dwFileAttributes))
        aResult = NFile::NDirectory::RemoveDirectoryWithSubItems(
           aPluginPanelItem.FindData.cFileName);
      else
        aResult = NFile::NDirectory::DeleteFileAlways(
           aPluginPanelItem.FindData.cFileName);
      if(!aResult)
        return NFileOperationReturnCode::kError;
    }
  }
  */
  return NFileOperationReturnCode::kSuccess;
}



/*
// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
*/

HRESULT CompressFiles(const CObjectVector<PluginPanelItem> &aPluginPanelItems)
{
  if (aPluginPanelItems.Size() == 0)
    return E_FAIL;

  UStringVector aFileNames;
  for(int i = 0; i < aPluginPanelItems.Size(); i++)
  {
    const PluginPanelItem &aPanelItem = aPluginPanelItems[i];
    CSysString aFullName;
    if (strcmp(aPanelItem.FindData.cFileName, "..") == 0 && 
        NFind::NAttributes::IsDirectory(aPanelItem.FindData.dwFileAttributes))
      return E_FAIL;
    if (strcmp(aPanelItem.FindData.cFileName, ".") == 0 && 
        NFind::NAttributes::IsDirectory(aPanelItem.FindData.dwFileAttributes))
      return E_FAIL;
    if (!MyGetFullPathName(aPanelItem.FindData.cFileName, aFullName))
      return E_FAIL;
    aFileNames.Add(MultiByteToUnicodeString(aFullName, CP_OEMCP));
  }

  NZipSettings::NCompression::CInfo aCompressionInfo;
  CZipRegistryManager aZipRegistryManager;
  aZipRegistryManager.ReadCompressionInfo(aCompressionInfo);
  if (!aCompressionInfo.MethodDefined)
    aCompressionInfo.Method = 1;
 
  int anArchiverIndex = 0;

  CObjectVector<NZipRootRegistry::CArchiverInfo> anArchiverInfoList;
  {
    CObjectVector<NZipRootRegistry::CArchiverInfo> aFullArchiverInfoList;
    NZipRootRegistry::ReadArchiverInfoList(aFullArchiverInfoList);
    for (int i = 0; i < aFullArchiverInfoList.Size(); i++)
    {
      const NZipRootRegistry::CArchiverInfo &anArchiverInfo = aFullArchiverInfoList[i];
      if (anArchiverInfo.UpdateEnabled)
      {
        if (anArchiverInfo.ClassID == aCompressionInfo.LastClassID && 
            aCompressionInfo.LastClassIDDefined)
          anArchiverIndex = anArchiverInfoList.Size();
        anArchiverInfoList.Add(anArchiverInfo);
      }
    }
  }
  if (anArchiverInfoList.IsEmpty())
    throw "There is no update achivers";


  UString aResultPath;
  {
    NName::CParsedPath aParsedPath;
    aParsedPath.ParsePath(aFileNames.Front());
    if(aParsedPath.PathParts.Size() == 0)
      return E_FAIL;
    if (aFileNames.Size() == 1 || aParsedPath.PathParts.Size() == 1)
    {
      CSysString aPureName, aDot, anExtension;
      aResultPath = aParsedPath.PathParts.Back();
    }
    else
    {
      aParsedPath.PathParts.DeleteBack();
      aResultPath = aParsedPath.PathParts.Back();
    }
  }
  CSysString anArchiveNameSrc = UnicodeStringToMultiByte(aResultPath, CP_OEMCP);
  CSysString anArchiveName = anArchiveNameSrc;

  const NZipRootRegistry::CArchiverInfo &anArchiverInfo = anArchiverInfoList[anArchiverIndex];
  int aPrevFormat = anArchiverIndex;
 
  if (!anArchiverInfo.KeepName)
  {
    int aDotPos = anArchiveName.ReverseFind('.');
    int aSlashPos = MyMax(anArchiveName.ReverseFind('\\'), anArchiveName.ReverseFind('/'));
    if (aDotPos > aSlashPos)
      anArchiveName = anArchiveName.Left(aDotPos);
  }
  anArchiveName += '.';
  anArchiveName += anArchiverInfo.Extension;
  
  const CActionSet *anActionSet = &kAddActionSet;

  while(true)
  {
    static const kYSize = 14;
    static const kXMid = 38;
  
    const kArchiveNameIndex = 2;
    const kMethodRadioIndex = kArchiveNameIndex + 2;
    const kModeRadioIndex = kMethodRadioIndex + 4;

    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = anArchiverInfoList[anArchiverIndex];

    char anUpdateAddToArchiveString[512];
    sprintf(anUpdateAddToArchiveString, 
        g_StartupInfo.GetMsgString(NMessageID::kUpdateAddToArchive), anArchiverInfo.Name);

    struct CInitDialogItem anInitItems[]=
    {
      { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kUpdateTitle, NULL, NULL },

      { DI_TEXT, 5, 2, 0, 0, false, false, 0, false, -1, anUpdateAddToArchiveString, NULL },
      
      { DI_EDIT, 5, 3, 70, 3, true, false, DIF_HISTORY, false, -1, anArchiveName, kArchiveHistoryKeyName},
      // { DI_EDIT, 5, 3, 70, 3, true, false, 0, false, -1, anArchiveName, NULL},
      
      { DI_SINGLEBOX, 4, 4, kXMid - 2, 4 + 4, false, false, 0, false, NMessageID::kUpdateMethod, NULL, NULL },
      { DI_RADIOBUTTON, 6, 5, 0, 0, false, 
          aCompressionInfo.Method == 0, 
          DIF_GROUP, false, NMessageID::kUpdateMethodStore, NULL, NULL },
      { DI_RADIOBUTTON, 6, 6, 0, 0, false, 
          aCompressionInfo.Method == 1, 
          0, false, NMessageID::kUpdateMethodNormal, NULL, NULL },
      { DI_RADIOBUTTON, 6, 7, 0, 0, false,
          aCompressionInfo.Method == 2, 
          false, 0, NMessageID::kUpdateMethodMaximum, NULL, NULL },
      
      { DI_SINGLEBOX, kXMid, 4, 70, 4 + 5, false, false, 0, false, NMessageID::kUpdateMode, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 5, 0, 0, false, 
          anActionSet == &kAddActionSet,
          DIF_GROUP, false, NMessageID::kUpdateModeAdd, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 6, 0, 0, false, 
          anActionSet == &kUpdateActionSet,
          0, false, NMessageID::kUpdateModeUpdate, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 7, 0, 0, false, 
          anActionSet == &kFreshActionSet,
          0, false, NMessageID::kUpdateModeFreshen, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 8, 0, 0, false, 
          anActionSet == &kSynchronizeActionSet,
          0, false, NMessageID::kUpdateModeSynchronize, NULL, NULL },
      
      { DI_TEXT, 3, kYSize - 4, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, "", NULL  },  
      
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kUpdateAdd, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kUpdateSelectArchiver, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL  }
    };

    const kNumDialogItems = sizeof(anInitItems) / sizeof(anInitItems[0]);
    
    const kOkButtonIndex = kNumDialogItems - 3;
    const kSelectarchiverButtonIndex = kNumDialogItems - 2;

    FarDialogItem aDialogItems[kNumDialogItems];
    g_StartupInfo.InitDialogItems(anInitItems, aDialogItems, kNumDialogItems);
    int anAskCode = g_StartupInfo.ShowDialog(76, kYSize, 
        kHelpTopic, aDialogItems, kNumDialogItems);

    anArchiveName = aDialogItems[kArchiveNameIndex].Data;
    anArchiveName.Trim();

    if (aDialogItems[kMethodRadioIndex].Selected)
      aCompressionInfo.SetMethod(0);
    else if (aDialogItems[kMethodRadioIndex + 1].Selected)
      aCompressionInfo.SetMethod(1);
    else if (aDialogItems[kMethodRadioIndex + 2].Selected)
      aCompressionInfo.SetMethod(2);
    else
      throw 51751;

    if (aDialogItems[kModeRadioIndex].Selected)
      anActionSet = &kAddActionSet;
    else if (aDialogItems[kModeRadioIndex + 1].Selected)
      anActionSet = &kUpdateActionSet;
    else if (aDialogItems[kModeRadioIndex + 2].Selected)
      anActionSet = &kFreshActionSet;
    else if (aDialogItems[kModeRadioIndex + 3].Selected)
      anActionSet = &kSynchronizeActionSet;
    else
      throw 51751;

    if (anAskCode == kSelectarchiverButtonIndex)
    {
      CSysStringVector aArchiverNames;
      for(int i = 0; i < anArchiverInfoList.Size(); i++)
        aArchiverNames.Add(anArchiverInfoList[i].Name);
    
      int anIndex = g_StartupInfo.Menu(FMENU_AUTOHIGHLIGHT, 
          g_StartupInfo.GetMsgString(NMessageID::kUpdateSelectArchiverMenuTitle),
          NULL, aArchiverNames, anArchiverIndex);
      if(anIndex >= 0)
      {
        const NZipRootRegistry::CArchiverInfo &aPrevArchiverInfo = anArchiverInfoList[aPrevFormat];
        if (aPrevArchiverInfo.KeepName)
        {
          const CSysString &aPrevExtension = aPrevArchiverInfo.Extension;
          const int aPrevExtensionLen = aPrevExtension.Length();
          if (anArchiveName.Right(aPrevExtensionLen).CompareNoCase(aPrevExtension) == 0)
          {
            int aPos = anArchiveName.Length() - aPrevExtensionLen;
            CSysString aTemp = anArchiveName.Left(aPos);
            if (aPos > 1)
            {
              int aDotPos = anArchiveName.ReverseFind('.');
              if (aDotPos == aPos - 1)
                anArchiveName = anArchiveName.Left(aDotPos);
            }
          }
        }

        anArchiverIndex = anIndex;
        const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
            anArchiverInfoList[anArchiverIndex];
        aPrevFormat = anArchiverIndex;
        
        if (anArchiverInfo.KeepName)
          anArchiveName = anArchiveNameSrc;
        else
        {
          int aDotPos = anArchiveName.ReverseFind('.');
          int aSlashPos = MyMax(anArchiveName.ReverseFind('\\'), anArchiveName.ReverseFind('/'));
          if (aDotPos > aSlashPos)
            anArchiveName = anArchiveName.Left(aDotPos);
        }
        anArchiveName += '.';
        anArchiveName += anArchiverInfo.Extension;
      }
      continue;
    }

    if (anAskCode != kOkButtonIndex)
      return E_ABORT;
    
    break;
  }

  const CLSID &aClassID = anArchiverInfoList[anArchiverIndex].ClassID;
  aCompressionInfo.SetLastClassID(aClassID);
  aZipRegistryManager.SaveCompressionInfo(aCompressionInfo);

  NZipSettings::NWorkDir::CInfo aWorkDirInfo;
  aZipRegistryManager.ReadWorkDirInfo(aWorkDirInfo);

  CSysString aFullArchiveName;
  if (!MyGetFullPathName(anArchiveName, aFullArchiveName))
    return E_FAIL;
   
  CSysString aWorkDir = GetWorkDir(aWorkDirInfo, aFullArchiveName);
  CreateComplexDirectory(aWorkDir);

  CTempFile aTempFile;
  CSysString aTempFileName;
  if (aTempFile.Create(aWorkDir, kTempArcivePrefix, aTempFileName) == 0)
    return E_FAIL;



  CScreenRestorer aScreenRestorer;
  CProgressBox aProgressBox;
  CProgressBox *aProgressBoxPointer = NULL;

  aScreenRestorer.Save();

  aProgressBoxPointer = &aProgressBox;
  aProgressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
     g_StartupInfo.GetMsgString(NMessageID::kUpdating), 1 << 16);


  // std::auto_ptr<CProxyHandler> aProxyHandler;
  NFind::CFileInfo aFileInfo;

  CComPtr<IOutArchiveHandler100> anOutArchive;

  CComPtr<IArchiveHandler100> anArchiveHandler;
  if(NFind::FindFile(aFullArchiveName, aFileInfo))
  {
    if (aFileInfo.IsDirectory())
      throw "There is Directory with such name";

    NZipRootRegistry::CArchiverInfo anArchiverInfoResult;
    UString aDefaultName;
    RETURN_IF_NOT_S_OK(OpenArchive(aFullArchiveName, 
        &anArchiveHandler, anArchiverInfoResult, aDefaultName, 
        NULL));

    if (anArchiverInfoResult.ClassID != aClassID)
      throw "Type of existing archive differs from specified type";
    HRESULT aResult = anArchiveHandler.QueryInterface(&anOutArchive);
    if(aResult != S_OK)
    {
      g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
      return E_FAIL;
    }
  }
  else
  {
    // HRESULT aResult = anOutArchive.CoCreateInstance(aClassID);
    CComObjectNoLock<CAgent> *anAgentSpec = new CComObjectNoLock<CAgent>;
    anOutArchive = anAgentSpec;

    /*
    HRESULT aResult = anOutArchive.CoCreateInstance(CLSID_CAgentArchiveHandler);
    if (aResult != S_OK)
    {
      g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
      return E_FAIL;
    }
    */
  }

  CRecordVector<const wchar_t *> aFileNamePointers;
  aFileNamePointers.Reserve(aFileNames.Size());
  for(i = 0; i < aFileNames.Size(); i++)
    aFileNamePointers.Add(aFileNames[i]);

  anOutArchive->SetFolder(NULL);
  anOutArchive->SetFiles(&aFileNamePointers.Front(), aFileNamePointers.Size());
  BYTE anActionSetByte[6];
  for (i = 0; i < 6; i++)
    anActionSetByte[i] = anActionSet->StateActions[i];

  CComObjectNoLock<CUpdateCallBack100Imp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CUpdateCallBack100Imp>;
  CComPtr<IUpdateCallback100> anUpdateCallBack(anUpdateCallBackSpec );
  
  anUpdateCallBackSpec->Init(anArchiveHandler, &aProgressBox);


  RETURN_IF_NOT_S_OK(SetOutProperties(anOutArchive, aCompressionInfo.Method));

  HRESULT aResult = anOutArchive->DoOperation(&aClassID,
      MultiByteToUnicodeString(aTempFileName, CP_OEMCP), anActionSetByte, 
      NULL, anUpdateCallBack);
  anUpdateCallBack.Release();
  anOutArchive.Release();

  if (aResult != S_OK)
  {
    ShowErrorMessage(aResult);
    return aResult;
  }
 
  if(anArchiveHandler)
  {
    anArchiveHandler->Close();
    if (!DeleteFileAlways(aFullArchiveName))
    {
      ShowLastErrorMessage();
      return NFileOperationReturnCode::kError;
    }
  }
  aTempFile.DisableDeleting();
  if (!MoveFile(aTempFileName, aFullArchiveName))
  {
    ShowLastErrorMessage();
    return E_FAIL;
  }
  
  return S_OK;
}

