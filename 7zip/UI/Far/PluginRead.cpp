// PluginRead.cpp

#include "StdAfx.h"

#include "Plugin.h"

#include "Messages.h"

#include "Common/StringConvert.h"

#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/Defs.h"

#include "../Common/ZipRegistry.h"

#include "ExtractEngine.h"


using namespace std;
using namespace NFar;
using namespace NWindows;

static const char *kHelpTopicExtrFromSevenZip =  "Extract";

static const char kDirDelimiter = '\\';

static const char *kExractPathHistoryName  = "7-ZipExtractPath"; 

HRESULT CPlugin::ExtractFiles(
    bool decompressAllItems,
    const UINT32 *indices, 
    UINT32 aNumIndexes, 
    bool silent,
    NExtractionMode::NPath::EEnum pathMode, 
    NExtractionMode::NOverwrite::EEnum overwriteMode,
    const CSysString &destPath,
    bool passwordIsDefined, const UString &password)
{
  CScreenRestorer screenRestorer;
  CProgressBox progressBox;
  CProgressBox *progressBoxPointer = NULL;
  if (!silent)
  {
    screenRestorer.Save();

    progressBoxPointer = &progressBox;
    progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kExtracting), 1 << 17);
  }


  CExtractCallBackImp *extractCallbackSpec = new CExtractCallBackImp;
  CMyComPtr<IFolderArchiveExtractCallback> extractCallback(extractCallbackSpec);
  
  extractCallbackSpec->Init(
      CP_OEMCP,
      progressBoxPointer,       
      /*
      GetDefaultName(m_FileName, m_ArchiverInfo.Extension),
      m_FileInfo.LastWriteTime, m_FileInfo.Attributes,
      */
      passwordIsDefined, password);

  if (decompressAllItems)
    return m_ArchiveHandler->Extract(pathMode, overwriteMode,
        MultiByteToUnicodeString(destPath, CP_OEMCP), BoolToInt(false), extractCallback);
  else
  {
    CMyComPtr<IArchiveFolder> archiveFolder;
    _folder.QueryInterface(IID_IArchiveFolder, &archiveFolder);

    return archiveFolder->Extract(indices, aNumIndexes, pathMode, overwriteMode,
        MultiByteToUnicodeString(destPath, CP_OEMCP), BoolToInt(false), extractCallback);
  }
}

NFileOperationReturnCode::EEnum CPlugin::GetFiles(struct PluginPanelItem *aPanelItems, 
    int itemsNumber, int aMove, char *_aDestPath, int anOpMode)
{
  return GetFilesReal(aPanelItems, itemsNumber, aMove, _aDestPath, anOpMode, (anOpMode & OPM_SILENT) == 0);
}

NFileOperationReturnCode::EEnum CPlugin::GetFilesReal(struct PluginPanelItem *aPanelItems, 
    int itemsNumber, int aMove, char *_aDestPath, int anOpMode, bool showBox)
{
  if(aMove != 0)
  {
    g_StartupInfo.ShowMessage(NMessageID::kMoveIsNotSupported);
    return NFileOperationReturnCode::kError;
  }

  CSysString destPath = _aDestPath;
  NFile::NName::NormalizeDirPathPrefix(destPath);

  bool extractSelectedFiles = true;
  NExtraction::CInfo extractionInfo;
  extractionInfo.PathMode = NExtraction::NPathMode::kCurrentPathnames;
  extractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kWithoutPrompt;

  bool silent = (anOpMode & OPM_SILENT) != 0;
  bool decompressAllItems = false;
  UString password = Password;
  bool passwordIsDefined = PasswordIsDefined;

  if (!silent)
  {
    const int kPathIndex = 2;

    // CZipRegistryManager aZipRegistryManager;
    ReadExtractionInfo(extractionInfo);

    const int kPathModeRadioIndex = 4;
    const int kOverwriteModeRadioIndex = kPathModeRadioIndex + 4;
    const int kFilesModeIndex = kOverwriteModeRadioIndex + 5;
    const int kYSize = 18;
    
    const int kXMid = 38;

    AString oemPassword = UnicodeStringToMultiByte(password, CP_OEMCP);
    
    struct CInitDialogItem initItems[]={
      { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kExtractTitle, NULL, NULL },
      { DI_TEXT, 5, 2, 0, 0, false, false, 0, false, NMessageID::kExtractTo, NULL, NULL },
      
      { DI_EDIT, 5, 3, 70, 3, true, false, DIF_HISTORY, false, -1, destPath, kExractPathHistoryName},
      // { DI_EDIT, 5, 3, 70, 3, true, false, 0, false, -1, destPath, NULL},
      
      { DI_SINGLEBOX, 4, 5, kXMid - 2, 5 + 4, false, false, 0, false, NMessageID::kExtractPathMode, NULL, NULL },
      { DI_RADIOBUTTON, 6, 6, 0, 0, false, 
          extractionInfo.PathMode == NExtraction::NPathMode::kFullPathnames, 
          DIF_GROUP, false, NMessageID::kExtractPathFull, NULL, NULL },
      { DI_RADIOBUTTON, 6, 7, 0, 0, false, 
          extractionInfo.PathMode == NExtraction::NPathMode::kCurrentPathnames,
          0, false, NMessageID::kExtractPathCurrent, NULL, NULL },
      { DI_RADIOBUTTON, 6, 8, 0, 0, false,
          extractionInfo.PathMode == NExtraction::NPathMode::kNoPathnames, 
          false, 0, NMessageID::kExtractPathNo, NULL, NULL },
      
      { DI_SINGLEBOX, kXMid, 5, 70, 5 + 5, false, false, 0, false, NMessageID::kExtractOwerwriteMode, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 6, 0, 0, false, 
          extractionInfo.OverwriteMode == NExtraction::NOverwriteMode::kAskBefore, 
          DIF_GROUP, false, NMessageID::kExtractOwerwriteAsk, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 7, 0, 0, false, 
          extractionInfo.OverwriteMode == NExtraction::NOverwriteMode::kWithoutPrompt, 
          0, false, NMessageID::kExtractOwerwritePrompt, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 8, 0, 0, false, 
          extractionInfo.OverwriteMode == NExtraction::NOverwriteMode::kSkipExisting, 
          0, false, NMessageID::kExtractOwerwriteSkip, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 9, 0, 0, false, 
          extractionInfo.OverwriteMode == NExtraction::NOverwriteMode::kAutoRename, 
          0, false, NMessageID::kExtractOwerwriteAutoRename, NULL, NULL },
      
      { DI_SINGLEBOX, 4, 10, kXMid- 2, 10 + 3, false, false, 0, false, NMessageID::kExtractFilesMode, NULL, NULL },
      { DI_RADIOBUTTON, 6, 11, 0, 0, false, true, DIF_GROUP, false, NMessageID::kExtractFilesSelected, NULL, NULL },
      { DI_RADIOBUTTON, 6, 12, 0, 0, false, false, 0, false, NMessageID::kExtractFilesAll, NULL, NULL },
      
      { DI_SINGLEBOX, kXMid, 11, 70, 11 + 2, false, false, 0, false, NMessageID::kExtractPassword, NULL, NULL },
      { DI_PSWEDIT, kXMid + 2, 12, 70 - 2, 12, false, false, 0, false, -1, oemPassword, NULL},
      
      { DI_TEXT, 3, kYSize - 4, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, "", NULL  },  
      
      
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kExtractExtract, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kExtractCancel, NULL, NULL  }
    };
   
    const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
    const int kOkButtonIndex = kNumDialogItems - 2;
    const int kPasswordIndex = kNumDialogItems - 4;

    FarDialogItem dialogItems[kNumDialogItems];
    g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);
    while(true)
    {
      int askCode = g_StartupInfo.ShowDialog(76, kYSize, 
        kHelpTopicExtrFromSevenZip, dialogItems, kNumDialogItems);
      if (askCode != kOkButtonIndex)
        return NFileOperationReturnCode::kInterruptedByUser;
      destPath = dialogItems[kPathIndex].Data;
      destPath.Trim();
      if (destPath.IsEmpty())
      {
        if(!NFile::NDirectory::MyGetCurrentDirectory(destPath))
          throw 318016;
        NFile::NName::NormalizeDirPathPrefix(destPath);
        break;
      }
      else
      {
        if(destPath[destPath.Length() - 1] == kDirDelimiter)
          break;
      }
      g_StartupInfo.ShowMessage("You must specify directory path");
    }

    if (dialogItems[kPathModeRadioIndex].Selected)
      extractionInfo.PathMode = NExtraction::NPathMode::kFullPathnames;
    else if (dialogItems[kPathModeRadioIndex + 1].Selected)
      extractionInfo.PathMode = NExtraction::NPathMode::kCurrentPathnames;
    else if (dialogItems[kPathModeRadioIndex + 2].Selected)
      extractionInfo.PathMode = NExtraction::NPathMode::kNoPathnames;
    else
      throw 31806;

    if (dialogItems[kOverwriteModeRadioIndex].Selected)
      extractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kAskBefore;
    else if (dialogItems[kOverwriteModeRadioIndex + 1].Selected)
      extractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kWithoutPrompt;
    else if (dialogItems[kOverwriteModeRadioIndex + 2].Selected)
      extractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kSkipExisting;
    else if (dialogItems[kOverwriteModeRadioIndex + 3].Selected)
      extractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kAutoRename;
    else
      throw 31806;
    
    if (dialogItems[kFilesModeIndex].Selected)
      decompressAllItems = false;
    else if (dialogItems[kFilesModeIndex + 1].Selected)
      decompressAllItems = true;
    else
      throw 31806;

    SaveExtractionInfo(extractionInfo);

    if (dialogItems[kFilesModeIndex].Selected)
      extractSelectedFiles = true;
    else if (dialogItems[kFilesModeIndex + 1].Selected)
      extractSelectedFiles = false;
    else
      throw 31806;

    oemPassword = dialogItems[kPasswordIndex].Data;
    password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
    passwordIsDefined = !password.IsEmpty();
  }

  NFile::NDirectory::CreateComplexDirectory(destPath);

  /*
  vector<int> aRealIndexes;
  if (!decompressAllItems)
    GetRealIndexes(aPanelItems, itemsNumber, aRealIndexes);
  */
  CRecordVector<UINT32> indices;
  indices.Reserve(itemsNumber);
  for (int i = 0; i < itemsNumber; i++)
    indices.Add(aPanelItems[i].UserData);

  NExtractionMode::NPath::EEnum pathMode;
  NExtractionMode::NOverwrite::EEnum overwriteMode;
  switch (extractionInfo.OverwriteMode)
  {
    case NExtraction::NOverwriteMode::kAskBefore:
      overwriteMode = NExtractionMode::NOverwrite::kAskBefore;
      break;
    case NExtraction::NOverwriteMode::kWithoutPrompt:
      overwriteMode = NExtractionMode::NOverwrite::kWithoutPrompt;
      break;
    case NExtraction::NOverwriteMode::kSkipExisting:
      overwriteMode = NExtractionMode::NOverwrite::kSkipExisting;
      break;
    case NExtraction::NOverwriteMode::kAutoRename:
      overwriteMode = NExtractionMode::NOverwrite::kAutoRename;
      break;
    default:
      throw 12334454;
  }
  switch (extractionInfo.PathMode)
  {
    case NExtraction::NPathMode::kFullPathnames:
      pathMode = NExtractionMode::NPath::kFullPathnames;
      break;
    case NExtraction::NPathMode::kCurrentPathnames:
      pathMode = NExtractionMode::NPath::kCurrentPathnames;
      break;
    case NExtraction::NPathMode::kNoPathnames:
      pathMode = NExtractionMode::NPath::kNoPathnames;
      break;
    default:
      throw 12334455;
  }
  HRESULT result = ExtractFiles(decompressAllItems, &indices.Front(), itemsNumber, 
      !showBox, pathMode, overwriteMode, destPath, passwordIsDefined, password);
  // HRESULT result = ExtractFiles(decompressAllItems, aRealIndexes, !showBox, 
  //     extractionInfo, destPath, passwordIsDefined, password);
  if (result != S_OK)
  {
    if (result == E_ABORT)
      return NFileOperationReturnCode::kInterruptedByUser;
    ShowErrorMessage(result);
    return NFileOperationReturnCode::kError;
  }

  // if(aMove != 0)
  // {
  //   if(DeleteFiles(aPanelItems, itemsNumber, anOpMode) == FALSE)
  //     return NFileOperationReturnCode::kError;
  // }
  return NFileOperationReturnCode::kSuccess;
}
