// PluginRead.cpp

#include "StdAfx.h"

#include "Plugin.h"

#include "Messages.h"

#include "Common/StringConvert.h"

#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"

#include "../../Archiver/Common/ZipRegistry.h"
#include "../../Archiver/Common/DefaultName.h"

#include "ExtractEngine.h"


using namespace std;
using namespace NFar;
using namespace NWindows;

static const char *kHelpTopicExtrFromSevenZip =  "Extract";

using namespace NZipSettings;


static const char kDirDelimiter = '\\';

static const char *kExractPathHistoryName  = "7-ZipExtractPath"; 

HRESULT CPlugin::ExtractFiles(
    bool aDecompressAllItems,
    const UINT32 *anIndexes, 
    UINT32 aNumIndexes, 
    bool aSilent,
    NExtractionMode::NPath::EEnum aPathMode, 
    NExtractionMode::NOverwrite::EEnum anOverwriteMode,
    const CSysString &aDestPath,
    bool aPasswordIsDefined, const UString &aPassword)
{
  CScreenRestorer aScreenRestorer;
  CProgressBox aProgressBox;
  CProgressBox *aProgressBoxPointer = NULL;
  if (!aSilent)
  {
    aScreenRestorer.Save();

    aProgressBoxPointer = &aProgressBox;
    aProgressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kExtracting), 1 << 17);
  }


  CComObjectNoLock<CExtractCallBackImp> *anExtractCallBackSpec =
      new CComObjectNoLock<CExtractCallBackImp>;
  CComPtr<IExtractCallback2> anExtractCallBack(anExtractCallBackSpec);
  
  anExtractCallBackSpec->Init(m_ArchiveHandler, 
      /*
      aDestPath, anExtractionInfo, aRemovePathParts, 
      */
      CP_OEMCP,
      aProgressBoxPointer,       
      /*
      GetDefaultName(m_FileName, m_ArchiverInfo.Extension),
      m_FileInfo.LastWriteTime, m_FileInfo.Attributes,
      */
      aPasswordIsDefined, aPassword);

  if (aDecompressAllItems)
    return m_ArchiveHandler->Extract(aPathMode, anOverwriteMode,
        MultiByteToUnicodeString(aDestPath, CP_OEMCP), BoolToMyBool(false), anExtractCallBack);
  else
    return m_ArchiveFolder->Extract(anIndexes, aNumIndexes, aPathMode, anOverwriteMode,
        MultiByteToUnicodeString(aDestPath, CP_OEMCP), BoolToMyBool(false), anExtractCallBack);
}

NFileOperationReturnCode::EEnum CPlugin::GetFiles(struct PluginPanelItem *aPanelItems, 
    int anItemsNumber, int aMove, char *_aDestPath, int anOpMode)
{
  return GetFilesReal(aPanelItems, anItemsNumber, aMove, _aDestPath, anOpMode, (anOpMode & OPM_SILENT) == 0);
}

NFileOperationReturnCode::EEnum CPlugin::GetFilesReal(struct PluginPanelItem *aPanelItems, 
    int anItemsNumber, int aMove, char *_aDestPath, int anOpMode, bool aShowBox)
{
  if(aMove != 0)
  {
    g_StartupInfo.ShowMessage(NMessageID::kMoveIsNotSupported);
    return NFileOperationReturnCode::kError;
  }

  CSysString aDestPath = _aDestPath;
  NFile::NName::NormalizeDirPathPrefix(aDestPath);

  bool anExtractSelectedFiles = true;
  NExtraction::CInfo anExtractionInfo;
  anExtractionInfo.PathMode = NExtraction::NPathMode::kCurrentPathnames;
  anExtractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kWithoutPrompt;

  bool aSilent = (anOpMode & OPM_SILENT) != 0;
  bool aDecompressAllItems = false;
  UString aPassword;
  bool aPasswordIsDefined = false;

  if (!aSilent)
  {
    const kPathIndex = 2;

    CZipRegistryManager aZipRegistryManager;
    aZipRegistryManager.ReadExtractionInfo(anExtractionInfo);

    const kPathModeRadioIndex = 4;
    const kOverwriteModeRadioIndex = kPathModeRadioIndex + 4;
    const kFilesModeIndex = kOverwriteModeRadioIndex + 5;
    static const kYSize = 18;
    
    static const kXMid = 38;
    
    struct CInitDialogItem anInitItems[]={
      { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kExtractTitle, NULL, NULL },
      { DI_TEXT, 5, 2, 0, 0, false, false, 0, false, NMessageID::kExtractTo, NULL, NULL },
      
      { DI_EDIT, 5, 3, 70, 3, true, false, DIF_HISTORY, false, -1, aDestPath, kExractPathHistoryName},
      // { DI_EDIT, 5, 3, 70, 3, true, false, 0, false, -1, aDestPath, NULL},
      
      { DI_SINGLEBOX, 4, 5, kXMid - 2, 5 + 4, false, false, 0, false, NMessageID::kExtractPathMode, NULL, NULL },
      { DI_RADIOBUTTON, 6, 6, 0, 0, false, 
          anExtractionInfo.PathMode == NExtraction::NPathMode::kFullPathnames, 
          DIF_GROUP, false, NMessageID::kExtractPathFull, NULL, NULL },
      { DI_RADIOBUTTON, 6, 7, 0, 0, false, 
          anExtractionInfo.PathMode == NExtraction::NPathMode::kCurrentPathnames,
          0, false, NMessageID::kExtractPathCurrent, NULL, NULL },
      { DI_RADIOBUTTON, 6, 8, 0, 0, false,
          anExtractionInfo.PathMode == NExtraction::NPathMode::kNoPathnames, 
          false, 0, NMessageID::kExtractPathNo, NULL, NULL },
      
      { DI_SINGLEBOX, kXMid, 5, 70, 5 + 5, false, false, 0, false, NMessageID::kExtractOwerwriteMode, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 6, 0, 0, false, 
          anExtractionInfo.OverwriteMode == NExtraction::NOverwriteMode::kAskBefore, 
          DIF_GROUP, false, NMessageID::kExtractOwerwriteAsk, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 7, 0, 0, false, 
          anExtractionInfo.OverwriteMode == NExtraction::NOverwriteMode::kWithoutPrompt, 
          0, false, NMessageID::kExtractOwerwritePrompt, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 8, 0, 0, false, 
          anExtractionInfo.OverwriteMode == NExtraction::NOverwriteMode::kSkipExisting, 
          0, false, NMessageID::kExtractOwerwriteSkip, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 9, 0, 0, false, 
          anExtractionInfo.OverwriteMode == NExtraction::NOverwriteMode::kAutoRename, 
          0, false, NMessageID::kExtractOwerwriteAutoRename, NULL, NULL },
      
      { DI_SINGLEBOX, 4, 10, kXMid- 2, 10 + 3, false, false, 0, false, NMessageID::kExtractFilesMode, NULL, NULL },
      { DI_RADIOBUTTON, 6, 11, 0, 0, false, true, DIF_GROUP, false, NMessageID::kExtractFilesSelected, NULL, NULL },
      { DI_RADIOBUTTON, 6, 12, 0, 0, false, false, 0, false, NMessageID::kExtractFilesAll, NULL, NULL },
      
      { DI_SINGLEBOX, kXMid, 11, 70, 11 + 2, false, false, 0, false, NMessageID::kExtractPassword, NULL, NULL },
      { DI_PSWEDIT, kXMid + 2, 12, 70 - 2, 12, false, false, 0, false, -1, "", NULL},
      
      { DI_TEXT, 3, kYSize - 4, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, "", NULL  },  
      
      
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kExtractExtract, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kExtractCancel, NULL, NULL  }
    };
   
    const kNumDialogItems = sizeof(anInitItems) / sizeof(anInitItems[0]);
    const kOkButtonIndex = kNumDialogItems - 2;
    const kPasswordIndex = kNumDialogItems - 4;

    FarDialogItem aDialogItems[kNumDialogItems];
    g_StartupInfo.InitDialogItems(anInitItems, aDialogItems, kNumDialogItems);
    while(true)
    {
      int anAskCode = g_StartupInfo.ShowDialog(76, kYSize, 
        kHelpTopicExtrFromSevenZip, aDialogItems, kNumDialogItems);
      if (anAskCode != kOkButtonIndex)
        return NFileOperationReturnCode::kInterruptedByUser;
      aDestPath = aDialogItems[kPathIndex].Data;
      aDestPath.Trim();
      if(!aDestPath.IsEmpty() &&
        aDestPath[aDestPath.Length() - 1] == kDirDelimiter)
        break;
      g_StartupInfo.ShowMessage("You must specify directory path");
    }

    if (aDialogItems[kPathModeRadioIndex].Selected)
      anExtractionInfo.PathMode = NExtraction::NPathMode::kFullPathnames;
    else if (aDialogItems[kPathModeRadioIndex + 1].Selected)
      anExtractionInfo.PathMode = NExtraction::NPathMode::kCurrentPathnames;
    else if (aDialogItems[kPathModeRadioIndex + 2].Selected)
      anExtractionInfo.PathMode = NExtraction::NPathMode::kNoPathnames;
    else
      throw 31806;

    if (aDialogItems[kOverwriteModeRadioIndex].Selected)
      anExtractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kAskBefore;
    else if (aDialogItems[kOverwriteModeRadioIndex + 1].Selected)
      anExtractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kWithoutPrompt;
    else if (aDialogItems[kOverwriteModeRadioIndex + 2].Selected)
      anExtractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kSkipExisting;
    else if (aDialogItems[kOverwriteModeRadioIndex + 3].Selected)
      anExtractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kAutoRename;
    else
      throw 31806;
    
    if (aDialogItems[kFilesModeIndex].Selected)
      aDecompressAllItems = false;
    else if (aDialogItems[kFilesModeIndex + 1].Selected)
      aDecompressAllItems = true;
    else
      throw 31806;

    aZipRegistryManager.SaveExtractionInfo(anExtractionInfo);

    if (aDialogItems[kFilesModeIndex].Selected)
      anExtractSelectedFiles = true;
    else if (aDialogItems[kFilesModeIndex + 1].Selected)
      anExtractSelectedFiles = false;
    else
      throw 31806;

    AString anOemPassword = aDialogItems[kPasswordIndex].Data;

    aPassword = MultiByteToUnicodeString(anOemPassword, CP_OEMCP); 

    aPasswordIsDefined = !aPassword.IsEmpty();
  }

  NFile::NDirectory::CreateComplexDirectory(aDestPath);

  /*
  vector<int> aRealIndexes;
  if (!aDecompressAllItems)
    GetRealIndexes(aPanelItems, anItemsNumber, aRealIndexes);
  */
  CRecordVector<UINT32> anIndexes;
  anIndexes.Reserve(anItemsNumber);
  for (int i = 0; i < anItemsNumber; i++)
    anIndexes.Add(aPanelItems[i].UserData);

  NExtractionMode::NPath::EEnum aPathMode;
  NExtractionMode::NOverwrite::EEnum anOverwriteMode;
  switch (anExtractionInfo.OverwriteMode)
  {
    case NExtraction::NOverwriteMode::kAskBefore:
      anOverwriteMode = NExtractionMode::NOverwrite::kAskBefore;
      break;
    case NExtraction::NOverwriteMode::kWithoutPrompt:
      anOverwriteMode = NExtractionMode::NOverwrite::kWithoutPrompt;
      break;
    case NExtraction::NOverwriteMode::kSkipExisting:
      anOverwriteMode = NExtractionMode::NOverwrite::kSkipExisting;
      break;
    case NExtraction::NOverwriteMode::kAutoRename:
      anOverwriteMode = NExtractionMode::NOverwrite::kAutoRename;
      break;
    default:
      throw 12334454;
  }
  switch (anExtractionInfo.PathMode)
  {
    case NExtraction::NPathMode::kFullPathnames:
      aPathMode = NExtractionMode::NPath::kFullPathnames;
      break;
    case NExtraction::NPathMode::kCurrentPathnames:
      aPathMode = NExtractionMode::NPath::kCurrentPathnames;
      break;
    case NExtraction::NPathMode::kNoPathnames:
      aPathMode = NExtractionMode::NPath::kNoPathnames;
      break;
    default:
      throw 12334455;
  }
  HRESULT aResult = ExtractFiles(aDecompressAllItems, &anIndexes.Front(), anItemsNumber, 
      !aShowBox, aPathMode, anOverwriteMode, aDestPath, aPasswordIsDefined, aPassword);
  // HRESULT aResult = ExtractFiles(aDecompressAllItems, aRealIndexes, !aShowBox, 
  //     anExtractionInfo, aDestPath, aPasswordIsDefined, aPassword);
  if (aResult != S_OK)
  {
    if (aResult == E_ABORT)
      return NFileOperationReturnCode::kInterruptedByUser;
    ShowErrorMessage(aResult);
    return NFileOperationReturnCode::kError;
  }

  // if(aMove != 0)
  // {
  //   if(DeleteFiles(aPanelItems, anItemsNumber, anOpMode) == FALSE)
  //     return NFileOperationReturnCode::kError;
  // }
  return NFileOperationReturnCode::kSuccess;
}
