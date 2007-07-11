// PluginWrite.cpp

#include "StdAfx.h"

#include "Plugin.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "../Common/ZipRegistry.h"
#include "../Common/WorkDir.h"
#include "../Common/OpenArchive.h"

#include "../Agent/Agent.h"

#include "ProgressBox.h"
#include "Messages.h"
#include "UpdateCallback100.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;
using namespace NFar;

using namespace NUpdateArchive;

static const char *kHelpTopic =  "Update";

static LPCWSTR kTempArcivePrefix = L"7zA";

static const char *kArchiveHistoryKeyName = "7-ZipArcName"; 

static UINT32 g_MethodMap[] = { 0, 1, 3, 5, 7, 9 }; 

static HRESULT SetOutProperties(IOutFolderArchive *outArchive, UINT32 method)
{
  CMyComPtr<ISetProperties> setProperties;
  if (outArchive->QueryInterface(IID_ISetProperties, (void **)&setProperties) == S_OK)
  {
    UStringVector realNames;
    realNames.Add(UString(L"x"));
    NCOM::CPropVariant value = (UInt32)method;
    CRecordVector<const wchar_t *> names;
    for(int i = 0; i < realNames.Size(); i++)
      names.Add(realNames[i]);
    RINOK(setProperties->SetProperties(&names.Front(), &value, names.Size()));
  }
  return S_OK;
}

NFileOperationReturnCode::EEnum CPlugin::PutFiles(
  struct PluginPanelItem *panelItems, int numItems, 
  int moveMode, int opMode)
{
  if(moveMode != 0)
  {
    g_StartupInfo.ShowMessage(NMessageID::kMoveIsNotSupported);
    return NFileOperationReturnCode::kError;
  }
  if (numItems == 0)
    return NFileOperationReturnCode::kError;

  /*
  if (!m_ArchiverInfo.UpdateEnabled)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return NFileOperationReturnCode::kError;
  }
  */

  const int kYSize = 14;
  const int kXMid = 38;

  NCompression::CInfo compressionInfo;
  ReadCompressionInfo(compressionInfo);

  int methodIndex = 0;
  int i;
  for (i = sizeof(g_MethodMap) / sizeof(g_MethodMap[0]) - 1; i >= 0; i--)
    if (compressionInfo.Level >= g_MethodMap[i])
    {
      methodIndex = i;
      break;
    }

  const int kMethodRadioIndex = 2;
  const int kModeRadioIndex = kMethodRadioIndex + 7;

  struct CInitDialogItem initItems[]={
    { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kUpdateTitle, NULL, NULL },
    { DI_SINGLEBOX, 4, 2, kXMid - 2, 2 + 7, false, false, 0, false, NMessageID::kUpdateMethod, NULL, NULL },
    { DI_RADIOBUTTON, 6, 3, 0, 0, methodIndex == 0, methodIndex == 0, 
        DIF_GROUP, false, NMessageID::kUpdateMethodStore, NULL, NULL },
    { DI_RADIOBUTTON, 6, 4, 0, 0, methodIndex == 1, methodIndex == 1, 
        0, false, NMessageID::kUpdateMethodFastest, NULL, NULL },
    { DI_RADIOBUTTON, 6, 5, 0, 0, methodIndex == 2, methodIndex == 2, 
        0, false, NMessageID::kUpdateMethodFast, NULL, NULL },
    { DI_RADIOBUTTON, 6, 6, 0, 0, methodIndex == 3, methodIndex == 3, 
        0, false, NMessageID::kUpdateMethodNormal, NULL, NULL },
    { DI_RADIOBUTTON, 6, 7, 0, 0, methodIndex == 4, methodIndex == 4, 
        0, false, NMessageID::kUpdateMethodMaximum, NULL, NULL },
    { DI_RADIOBUTTON, 6, 8, 0, 0, methodIndex == 5, methodIndex == 5, 
        0, false, NMessageID::kUpdateMethodUltra, NULL, NULL },
    
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
  
  const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
  const int kOkButtonIndex = kNumDialogItems - 2;
  FarDialogItem dialogItems[kNumDialogItems];
  g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);
  int askCode = g_StartupInfo.ShowDialog(76, kYSize, 
      kHelpTopic, dialogItems, kNumDialogItems);
  if (askCode != kOkButtonIndex)
    return NFileOperationReturnCode::kInterruptedByUser;

  compressionInfo.Level = g_MethodMap[0];
  for (i = 0; i < sizeof(g_MethodMap)/ sizeof(g_MethodMap[0]); i++)
    if (dialogItems[kMethodRadioIndex + i].Selected)
      compressionInfo.Level = g_MethodMap[i];

  const CActionSet *actionSet;

  if (dialogItems[kModeRadioIndex].Selected)
    actionSet = &kAddActionSet;
  else if (dialogItems[kModeRadioIndex + 1].Selected)
    actionSet = &kUpdateActionSet;
  else if (dialogItems[kModeRadioIndex + 2].Selected)
      actionSet = &kFreshActionSet;
  else if (dialogItems[kModeRadioIndex + 3].Selected)
      actionSet = &kSynchronizeActionSet;
  else
    throw 51751;

  SaveCompressionInfo(compressionInfo);

  NWorkDir::CInfo workDirInfo;
  ReadWorkDirInfo(workDirInfo);
  UString workDir = GetWorkDir(workDirInfo, m_FileName);
  CreateComplexDirectory(workDir);

  CTempFileW tempFile;
  UString tempFileName;
  if (tempFile.Create(workDir, kTempArcivePrefix, tempFileName) == 0)
    return NFileOperationReturnCode::kError;


  /*
  CSysStringVector fileNames;
  for(int i = 0; i < numItems; i++)
  {
    const PluginPanelItem &panelItem = panelItems[i];
    CSysString fullName;
    if (!MyGetFullPathName(panelItem.FindData.cFileName, fullName))
      return NFileOperationReturnCode::kError;
    fileNames.Add(fullName);
  }
  */

  CScreenRestorer screenRestorer;
  CProgressBox progressBox;
  CProgressBox *progressBoxPointer = NULL;
  if ((opMode & OPM_SILENT) == 0 && (opMode & OPM_FIND ) == 0)
  {
    screenRestorer.Save();

    progressBoxPointer = &progressBox;
    progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
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

  UStringVector fileNames;
  fileNames.Reserve(numItems);
  for(i = 0; i < numItems; i++)
    fileNames.Add(MultiByteToUnicodeString(panelItems[i].FindData.cFileName, CP_OEMCP));
  CRecordVector<const wchar_t *> fileNamePointers;
  fileNamePointers.Reserve(numItems);
  for(i = 0; i < numItems; i++)
    fileNamePointers.Add(fileNames[i]);

  CMyComPtr<IOutFolderArchive> outArchive;
  HRESULT result = m_ArchiveHandler.QueryInterface(IID_IOutFolderArchive, &outArchive);
  if(result != S_OK)
  {
    g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
    return NFileOperationReturnCode::kError;
  }
  outArchive->SetFolder(_folder);

  // CSysString aCurrentFolder;
  // MyGetCurrentDirectory(aCurrentFolder);
  // outArchive->SetFiles(MultiByteToUnicodeString(aCurrentFolder, CP_OEMCP), 
  outArchive->SetFiles(L"", 
      &fileNamePointers.Front(), fileNamePointers.Size());
  BYTE actionSetByte[NUpdateArchive::NPairState::kNumValues];
  for (i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
    actionSetByte[i] = (BYTE)actionSet->StateActions[i];

  CUpdateCallback100Imp *updateCallbackSpec = new CUpdateCallback100Imp;
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback(updateCallbackSpec );
  
  updateCallbackSpec->Init(m_ArchiveHandler, &progressBox);

  if (SetOutProperties(outArchive, compressionInfo.Level) != S_OK)
    return NFileOperationReturnCode::kError;

  result = outArchive->DoOperation2(tempFileName, actionSetByte, NULL, updateCallback);
  updateCallback.Release();
  outArchive.Release();

  /*
  HRESULT result = Compress(fileNames, anArchivePrefix, *actionSet, 
      m_ProxyHandler.get(), 
      m_ArchiverInfo.ClassID, compressionInfo.Method == 0,
      compressionInfo.Method == 2, tempFileName, progressBoxPointer);
  */

  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return NFileOperationReturnCode::kError;
  }

  _folder.Release();
  m_ArchiveHandler->Close();
  
  // m_FolderItem = NULL;
  
  if (!DeleteFileAlways(m_FileName))
  {
    ShowLastErrorMessage();
    return NFileOperationReturnCode::kError;
  }

  tempFile.DisableDeleting();
  if (!MyMoveFile(tempFileName, m_FileName))
  {
    ShowLastErrorMessage();
    return NFileOperationReturnCode::kError;
  }
  
  m_ArchiveHandler->ReOpen(NULL);
  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return NFileOperationReturnCode::kError;
  }

  /*
  if(m_ProxyHandler->ReInit(NULL) != S_OK)
    return NFileOperationReturnCode::kError;
  */
  
  ////////////////////////////
  // Restore FolderItem;

  m_ArchiveHandler->BindToRootFolder(&_folder);
  for (i = 0; i < aPathVector.Size(); i++)
  {
    CMyComPtr<IFolderFolder> newFolder;
    _folder->BindToFolder(aPathVector[i], &newFolder);
    if(!newFolder  )
      break;
    _folder = newFolder;
  }

  /*
  if(moveMode != 0)
  {
    for(int i = 0; i < numItems; i++)
    {
      const PluginPanelItem &aPluginPanelItem = panelItems[i];
      bool result;
      if(NFile::NFind::NAttributes::IsDirectory(aPluginPanelItem.FindData.dwFileAttributes))
        result = NFile::NDirectory::RemoveDirectoryWithSubItems(
           aPluginPanelItem.FindData.cFileName);
      else
        result = NFile::NDirectory::DeleteFileAlways(
           aPluginPanelItem.FindData.cFileName);
      if(!result)
        return NFileOperationReturnCode::kError;
    }
  }
  */
  return NFileOperationReturnCode::kSuccess;
}

namespace NPathType
{
  enum EEnum
  {
    kLocal,
    kUNC
  };
  EEnum GetPathType(const UString &path);
}

struct CParsedPath
{
  UString Prefix; // Disk or UNC with slash
  UStringVector PathParts;
  void ParsePath(const UString &path);
  UString MergePath() const;
};

static const wchar_t kDirDelimiter = WCHAR_PATH_SEPARATOR;
static const wchar_t kDiskDelimiter = L':';

namespace NPathType
{
  EEnum GetPathType(const UString &path)
  {
    if (path.Length() <= 2)
      return kLocal;
    if (path[0] == kDirDelimiter && path[1] == kDirDelimiter)
      return kUNC;
    return kLocal;
  }
}

void CParsedPath::ParsePath(const UString &path)
{
  int curPos = 0;
  switch (NPathType::GetPathType(path))
  {
    case NPathType::kLocal:
    {
      int posDiskDelimiter = path.Find(kDiskDelimiter);
      if(posDiskDelimiter >= 0)
      {
        curPos = posDiskDelimiter + 1;
        if (path.Length() > curPos)
          if(path[curPos] == kDirDelimiter)
            curPos++;
      }
      break;
    }
    case NPathType::kUNC:
    {
      int curPos = path.Find(kDirDelimiter, 2);
      if(curPos < 0)
        curPos = path.Length();
      else
        curPos++;
    }
  }
  Prefix = path.Left(curPos);
  SplitPathToParts(path.Mid(curPos), PathParts);
}

UString CParsedPath::MergePath() const
{
  UString result = Prefix;
  for(int i = 0; i < PathParts.Size(); i++)
  {
    if (i != 0)
      result += kDirDelimiter;
    result += PathParts[i];
  }
  return result;
}


/*
// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
*/

HRESULT CompressFiles(const CObjectVector<PluginPanelItem> &pluginPanelItems)
{
  if (pluginPanelItems.Size() == 0)
    return E_FAIL;

  UStringVector fileNames;
  int i;
  for(i = 0; i < pluginPanelItems.Size(); i++)
  {
    const PluginPanelItem &panelItem = pluginPanelItems[i];
    CSysString fullName;
    if (strcmp(panelItem.FindData.cFileName, "..") == 0 && 
        NFind::NAttributes::IsDirectory(panelItem.FindData.dwFileAttributes))
      return E_FAIL;
    if (strcmp(panelItem.FindData.cFileName, ".") == 0 && 
        NFind::NAttributes::IsDirectory(panelItem.FindData.dwFileAttributes))
      return E_FAIL;
    if (!MyGetFullPathName(panelItem.FindData.cFileName, fullName))
      return E_FAIL;
    fileNames.Add(MultiByteToUnicodeString(fullName, CP_OEMCP));
  }

  NCompression::CInfo compressionInfo;
  // CZipRegistryManager aZipRegistryManager;
  ReadCompressionInfo(compressionInfo);
  
  int archiverIndex = 0;

  CCodecs *codecs = new CCodecs;
  CMyComPtr<ICompressCodecsInfo> compressCodecsInfo = codecs;
  if (codecs->Load() != S_OK)
    throw "Can't load 7-Zip codecs";
  {
    for (int i = 0; i < codecs->Formats.Size(); i++)
    {
      const CArcInfoEx &arcInfo = codecs->Formats[i];
      if (arcInfo.UpdateEnabled)
      {
        if (archiverIndex == -1)
          archiverIndex = i;
        if (arcInfo.Name.CompareNoCase(compressionInfo.ArchiveType) == 0)
          archiverIndex = i;
      }
    }
  }


  UString resultPath;
  {
    CParsedPath parsedPath;
    parsedPath.ParsePath(fileNames.Front());
    if(parsedPath.PathParts.Size() == 0)
      return E_FAIL;
    if (fileNames.Size() == 1 || parsedPath.PathParts.Size() == 1)
    {
      // CSysString pureName, dot, extension;
      resultPath = parsedPath.PathParts.Back();
    }
    else
    {
      parsedPath.PathParts.DeleteBack();
      resultPath = parsedPath.PathParts.Back();
    }
  }
  UString archiveNameSrc = resultPath;
  UString archiveName = archiveNameSrc;

  const CArcInfoEx &arcInfo = codecs->Formats[archiverIndex];
  int prevFormat = archiverIndex;
 
  if (!arcInfo.KeepName)
  {
    int dotPos = archiveName.ReverseFind('.');
    int slashPos = MyMax(archiveName.ReverseFind('\\'), archiveName.ReverseFind('/'));
    if (dotPos > slashPos)
      archiveName = archiveName.Left(dotPos);
  }
  archiveName += L'.';
  archiveName += arcInfo.GetMainExt();
  
  const CActionSet *actionSet = &kAddActionSet;

  for (;;)
  {
    AString archiveNameA = UnicodeStringToMultiByte(archiveName, CP_OEMCP);
    const int kYSize = 16;
    const int kXMid = 38;
  
    const int kArchiveNameIndex = 2;
    const int kMethodRadioIndex = kArchiveNameIndex + 2;
    const int kModeRadioIndex = kMethodRadioIndex + 7;

    const CArcInfoEx &arcInfo = codecs->Formats[archiverIndex];

    char updateAddToArchiveString[512];
    const AString s = GetSystemString(arcInfo.Name, CP_OEMCP);

    sprintf(updateAddToArchiveString, 
        g_StartupInfo.GetMsgString(NMessageID::kUpdateAddToArchive), (const char *)s);

    int methodIndex = 0;
    int i;
    for (i = sizeof(g_MethodMap) / sizeof(g_MethodMap[0]) - 1; i >= 0; i--)
      if (compressionInfo.Level >= g_MethodMap[i])
      {
        methodIndex = i;
        break;
      }

    struct CInitDialogItem initItems[]=
    {
      { DI_DOUBLEBOX, 3, 1, 72, kYSize - 2, false, false, 0, false, NMessageID::kUpdateTitle, NULL, NULL },

      { DI_TEXT, 5, 2, 0, 0, false, false, 0, false, -1, updateAddToArchiveString, NULL },
      
      { DI_EDIT, 5, 3, 70, 3, true, false, DIF_HISTORY, false, -1, archiveNameA, kArchiveHistoryKeyName},
      // { DI_EDIT, 5, 3, 70, 3, true, false, 0, false, -1, archiveName, NULL},
      
      { DI_SINGLEBOX, 4, 4, kXMid - 2, 4 + 7, false, false, 0, false, NMessageID::kUpdateMethod, NULL, NULL },
      { DI_RADIOBUTTON, 6, 5, 0, 0, false, methodIndex == 0, 
          DIF_GROUP, false, NMessageID::kUpdateMethodStore, NULL, NULL },
      { DI_RADIOBUTTON, 6, 6, 0, 0, false, methodIndex == 1, 
          0, false, NMessageID::kUpdateMethodFastest, NULL, NULL },
      { DI_RADIOBUTTON, 6, 7, 0, 0, false, methodIndex == 2, 
          0, false, NMessageID::kUpdateMethodFast, NULL, NULL },
      { DI_RADIOBUTTON, 6, 8, 0, 0, false, methodIndex == 3, 
          0, false, NMessageID::kUpdateMethodNormal, NULL, NULL },
      { DI_RADIOBUTTON, 6, 9, 0, 0, false, methodIndex == 4, 
          false, 0, NMessageID::kUpdateMethodMaximum, NULL, NULL },
      { DI_RADIOBUTTON, 6, 10, 0, 0, false, methodIndex == 5, 
          false, 0, NMessageID::kUpdateMethodUltra, NULL, NULL },
      
      { DI_SINGLEBOX, kXMid, 4, 70, 4 + 5, false, false, 0, false, NMessageID::kUpdateMode, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 5, 0, 0, false, 
          actionSet == &kAddActionSet,
          DIF_GROUP, false, NMessageID::kUpdateModeAdd, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 6, 0, 0, false, 
          actionSet == &kUpdateActionSet,
          0, false, NMessageID::kUpdateModeUpdate, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 7, 0, 0, false, 
          actionSet == &kFreshActionSet,
          0, false, NMessageID::kUpdateModeFreshen, NULL, NULL },
      { DI_RADIOBUTTON, kXMid + 2, 8, 0, 0, false, 
          actionSet == &kSynchronizeActionSet,
          0, false, NMessageID::kUpdateModeSynchronize, NULL, NULL },
      
      { DI_TEXT, 3, kYSize - 4, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, "", NULL  },  
      
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, true, NMessageID::kUpdateAdd, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kUpdateSelectArchiver, NULL, NULL  },
      { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kCancel, NULL, NULL  }
    };

    const int kNumDialogItems = sizeof(initItems) / sizeof(initItems[0]);
    
    const int kOkButtonIndex = kNumDialogItems - 3;
    const int kSelectarchiverButtonIndex = kNumDialogItems - 2;

    FarDialogItem dialogItems[kNumDialogItems];
    g_StartupInfo.InitDialogItems(initItems, dialogItems, kNumDialogItems);
    int askCode = g_StartupInfo.ShowDialog(76, kYSize, 
        kHelpTopic, dialogItems, kNumDialogItems);

    archiveNameA = dialogItems[kArchiveNameIndex].Data;
    archiveNameA.Trim();
    archiveName = MultiByteToUnicodeString(archiveNameA, CP_OEMCP);

    compressionInfo.Level = g_MethodMap[0];
    for (i = 0; i < sizeof(g_MethodMap)/ sizeof(g_MethodMap[0]); i++)
      if (dialogItems[kMethodRadioIndex + i].Selected)
        compressionInfo.Level = g_MethodMap[i];

    if (dialogItems[kModeRadioIndex].Selected)
      actionSet = &kAddActionSet;
    else if (dialogItems[kModeRadioIndex + 1].Selected)
      actionSet = &kUpdateActionSet;
    else if (dialogItems[kModeRadioIndex + 2].Selected)
      actionSet = &kFreshActionSet;
    else if (dialogItems[kModeRadioIndex + 3].Selected)
      actionSet = &kSynchronizeActionSet;
    else
      throw 51751;

    if (askCode == kSelectarchiverButtonIndex)
    {
      CIntVector indices;
      CSysStringVector archiverNames;
      for(int i = 0; i < codecs->Formats.Size(); i++)
      {
        const CArcInfoEx &arc = codecs->Formats[i];
        if (arc.UpdateEnabled)
        {
          indices.Add(i);
          archiverNames.Add(GetSystemString(arc.Name, CP_OEMCP));
        }
      }
    
      int index = g_StartupInfo.Menu(FMENU_AUTOHIGHLIGHT, 
          g_StartupInfo.GetMsgString(NMessageID::kUpdateSelectArchiverMenuTitle),
          NULL, archiverNames, archiverIndex);
      if(index >= 0)
      {
        const CArcInfoEx &prevArchiverInfo = codecs->Formats[prevFormat];
        if (prevArchiverInfo.KeepName)
        {
          const UString &prevExtension = prevArchiverInfo.GetMainExt();
          const int prevExtensionLen = prevExtension.Length();
          if (archiveName.Right(prevExtensionLen).CompareNoCase(prevExtension) == 0)
          {
            int pos = archiveName.Length() - prevExtensionLen;
            if (pos > 1)
            {
              int dotPos = archiveName.ReverseFind('.');
              if (dotPos == pos - 1)
                archiveName = archiveName.Left(dotPos);
            }
          }
        }

        archiverIndex = indices[index];
        const CArcInfoEx &arcInfo = codecs->Formats[archiverIndex];
        prevFormat = archiverIndex;
        
        if (arcInfo.KeepName)
          archiveName = archiveNameSrc;
        else
        {
          int dotPos = archiveName.ReverseFind('.');
          int slashPos = MyMax(archiveName.ReverseFind('\\'), archiveName.ReverseFind('/'));
          if (dotPos > slashPos)
            archiveName = archiveName.Left(dotPos);
        }
        archiveName += L'.';
        archiveName += arcInfo.GetMainExt();
      }
      continue;
    }

    if (askCode != kOkButtonIndex)
      return E_ABORT;
    
    break;
  }

  const CArcInfoEx &archiverInfoFinal = codecs->Formats[archiverIndex];
  compressionInfo.ArchiveType = archiverInfoFinal.Name;
  SaveCompressionInfo(compressionInfo);

  NWorkDir::CInfo workDirInfo;
  ReadWorkDirInfo(workDirInfo);

  UString fullArchiveName;
  if (!MyGetFullPathName(archiveName, fullArchiveName))
    return E_FAIL;
   
  UString workDir = GetWorkDir(workDirInfo, fullArchiveName);
  CreateComplexDirectory(workDir);

  CTempFileW tempFile;
  UString tempFileName;
  if (tempFile.Create(workDir, kTempArcivePrefix, tempFileName) == 0)
    return E_FAIL;


  CScreenRestorer screenRestorer;
  CProgressBox progressBox;
  CProgressBox *progressBoxPointer = NULL;

  screenRestorer.Save();

  progressBoxPointer = &progressBox;
  progressBox.Init(g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
     g_StartupInfo.GetMsgString(NMessageID::kUpdating), 1 << 16);


  NFind::CFileInfoW fileInfo;

  CMyComPtr<IOutFolderArchive> outArchive;

  CMyComPtr<IInFolderArchive> archiveHandler;
  if(NFind::FindFile(fullArchiveName, fileInfo))
  {
    if (fileInfo.IsDirectory())
      throw "There is Directory with such name";

    CAgent *agentSpec = new CAgent;
    archiveHandler = agentSpec;
    // CLSID realClassID;
    CMyComBSTR archiveType;
    RINOK(agentSpec->Open(
        GetUnicodeString(fullArchiveName, CP_OEMCP), 
        // &realClassID, 
        &archiveType,
        NULL));

    if (archiverInfoFinal.Name.CompareNoCase((const wchar_t *)archiveType) != 0)
      throw "Type of existing archive differs from specified type";
    HRESULT result = archiveHandler.QueryInterface(
        IID_IOutFolderArchive, &outArchive);
    if(result != S_OK)
    {
      g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
      return E_FAIL;
    }
  }
  else
  {
    // HRESULT result = outArchive.CoCreateInstance(classID);
    CAgent *agentSpec = new CAgent;
    outArchive = agentSpec;

    /*
    HRESULT result = outArchive.CoCreateInstance(CLSID_CAgentArchiveHandler);
    if (result != S_OK)
    {
      g_StartupInfo.ShowMessage(NMessageID::kUpdateNotSupportedForThisArchive);
      return E_FAIL;
    }
    */
  }

  CRecordVector<const wchar_t *> fileNamePointers;
  fileNamePointers.Reserve(fileNames.Size());
  for(i = 0; i < fileNames.Size(); i++)
    fileNamePointers.Add(fileNames[i]);

  outArchive->SetFolder(NULL);
  // CSysString aCurrentFolder;
  // MyGetCurrentDirectory(aCurrentFolder);
  // outArchive->SetFiles(MultiByteToUnicodeString(aCurrentFolder, CP_OEMCP), 
  outArchive->SetFiles(L"", 
    &fileNamePointers.Front(), fileNamePointers.Size());
  BYTE actionSetByte[NUpdateArchive::NPairState::kNumValues];
  for (i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
    actionSetByte[i] = (BYTE)actionSet->StateActions[i];

  CUpdateCallback100Imp *updateCallbackSpec = new CUpdateCallback100Imp;
  CMyComPtr<IFolderArchiveUpdateCallback> updateCallback(updateCallbackSpec );
  
  updateCallbackSpec->Init(archiveHandler, &progressBox);


  RINOK(SetOutProperties(outArchive, compressionInfo.Level));

  HRESULT result = outArchive->DoOperation(
      codecs, archiverIndex,
      tempFileName, actionSetByte, 
      NULL, updateCallback);
  updateCallback.Release();
  outArchive.Release();

  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return result;
  }
 
  if(archiveHandler)
  {
    archiveHandler->Close();
    if (!DeleteFileAlways(fullArchiveName))
    {
      ShowLastErrorMessage();
      return NFileOperationReturnCode::kError;
    }
  }
  tempFile.DisableDeleting();
  if (!MyMoveFile(tempFileName, fullArchiveName))
  {
    ShowLastErrorMessage();
    return E_FAIL;
  }
  
  return S_OK;
}

