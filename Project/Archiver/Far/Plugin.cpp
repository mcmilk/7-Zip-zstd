// Plugin.cpp

#include "StdAfx.h"

#include "Plugin.h"

// #include "Windows/Time.h"
#include "Windows/FileName.h"
#include "Windows/FileDir.h"

#include "Common/StringConvert.h"

#include "Windows/PropVariantConversions.h"

#include "Far/FarUtils.h"

#include "../../Archiver/Common/PropIDUtils.h"

#include "Common/WildCard.h"

#include "Messages.h"

using namespace NWindows;
using namespace NFar;

CPlugin::CPlugin(const CSysString &fileName, 
    const UString &defaultName, 
    IInFolderArchive *archiveHandler,
    const NZipRootRegistry::CArchiverInfo &archiverInfo):
  m_ArchiveHandler(archiveHandler),
  m_FileName(fileName),
  m_DefaultName(defaultName),
  m_ArchiverInfo(archiverInfo)
{
  if (!NFile::NFind::FindFile(m_FileName, m_FileInfo))
    throw "error";
  archiveHandler->BindToRootFolder(&_folder);
}

CPlugin::~CPlugin()
{
}

static void MyGetFileTime(IFolderFolder *anArchiveFolder, UINT32 itemIndex,
    PROPID propID, FILETIME &fileTime)
{
  NCOM::CPropVariant propVariant;
  if (anArchiveFolder->GetProperty(itemIndex, propID, &propVariant) != S_OK)
    throw 271932;
  if (propVariant.vt == VT_EMPTY)
  {
    fileTime.dwHighDateTime = 0;
    fileTime.dwLowDateTime = 0;
  }
  else 
  {
    if (propVariant.vt != VT_FILETIME)
      throw 4191730;
    fileTime = propVariant.filetime;
  }
}
  
void CPlugin::ReadPluginPanelItem(PluginPanelItem &panelItem, UINT32 itemIndex)
{
  NCOM::CPropVariant propVariant;
  if (_folder->GetProperty(itemIndex, kpidName, &propVariant) != S_OK)
    throw 271932;

  if (propVariant.vt != VT_BSTR)
    throw 272340;

  CSysString oemString = UnicodeStringToMultiByte(propVariant.bstrVal, CP_OEMCP);
  strcpy(panelItem.FindData.cFileName, oemString);
  panelItem.FindData.cAlternateFileName[0] = 0;

  if (_folder->GetProperty(itemIndex, kpidAttributes, &propVariant) != S_OK)
    throw 271932;
  if (propVariant.vt == VT_UI4)
    panelItem.FindData.dwFileAttributes  = propVariant.ulVal;
  else if (propVariant.vt == VT_EMPTY)
    panelItem.FindData.dwFileAttributes = m_FileInfo.Attributes;
  else
    throw 21631;

  if (_folder->GetProperty(itemIndex, kpidIsFolder, &propVariant) != S_OK)
    throw 271932;
  if (propVariant.vt == VT_BOOL)
  {
    if (VARIANT_BOOLToBool(propVariant.boolVal))
      panelItem.FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  }
  else if (propVariant.vt != VT_EMPTY)
    throw 21632;

  if (_folder->GetProperty(itemIndex, kpidSize, &propVariant) != S_OK)
    throw 271932;
  UINT64 length;
  if (propVariant.vt == VT_EMPTY)
    length = 0;
  else
    length = ::ConvertPropVariantToUINT64(propVariant);
  panelItem.FindData.nFileSizeLow = UINT32(length);
  panelItem.FindData.nFileSizeHigh = UINT32(length >> 32);

  MyGetFileTime(_folder, itemIndex, kpidCreationTime, panelItem.FindData.ftCreationTime);
  MyGetFileTime(_folder, itemIndex, kpidLastAccessTime, panelItem.FindData.ftLastAccessTime);
  MyGetFileTime(_folder, itemIndex, kpidLastWriteTime, panelItem.FindData.ftLastWriteTime);

  if (panelItem.FindData.ftLastWriteTime.dwHighDateTime == 0 && 
      panelItem.FindData.ftLastWriteTime.dwLowDateTime == 0)
    panelItem.FindData.ftLastWriteTime = m_FileInfo.LastWriteTime;

  if (_folder->GetProperty(itemIndex, kpidPackedSize, &propVariant) != S_OK)
    throw 271932;
  if (propVariant.vt == VT_EMPTY)
    length = 0;
  else
    length = ::ConvertPropVariantToUINT64(propVariant);
  panelItem.PackSize = UINT32(length);
  panelItem.PackSizeHigh = UINT32(length >> 32);

  panelItem.Flags = 0;
  panelItem.NumberOfLinks = 0;

  panelItem.Description = NULL;
  panelItem.Owner = NULL;
  panelItem.CustomColumnData = NULL;
  panelItem.CustomColumnNumber = 0;

  panelItem.Reserved[0] = 0;
  panelItem.Reserved[1] = 0;
  panelItem.Reserved[2] = 0;
  

}

int CPlugin::GetFindData(PluginPanelItem **panelItems, 
    int *itemsNumber, int opMode)
{
  // CScreenRestorer screenRestorer;
  if ((opMode & OPM_SILENT) == 0 && (opMode & OPM_FIND ) == 0)
  {
    /*
    screenRestorer.Save();
    const char *aMsgItems[]=
    {
      g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kReadingList)
    };
    g_StartupInfo.ShowMessage(0, NULL, aMsgItems,
      sizeof(aMsgItems) / sizeof(aMsgItems[0]), 0);
    */
  }

  UINT32 numItems;
  _folder->GetNumberOfItems(&numItems);
  *panelItems = new PluginPanelItem[numItems];
  try
  {
    for(int i = 0; i < numItems; i++)
    {
      PluginPanelItem &panelItem = (*panelItems)[i];
      ReadPluginPanelItem(panelItem, i);
      panelItem.UserData = i;
    }
  }
  catch(...)
  {
    delete [](*panelItems);
    throw;
  }
  *itemsNumber = numItems;
  return(TRUE);
}

void CPlugin::FreeFindData(struct PluginPanelItem *panelItems,
    int itemsNumber)
{
  for(int i = 0; i < itemsNumber; i++)
    if(panelItems[i].Description != NULL)
      delete []panelItems[i].Description;
  delete []panelItems;
}


void CPlugin::EnterToDirectory(const UString &aDirName)
{
  CComPtr<IFolderFolder> newFolder;
  _folder->BindToFolder(aDirName, &newFolder);
  if(newFolder == NULL)
    if (aDirName.IsEmpty())
      return;
    else
      throw 40325;
  _folder = newFolder;
}

int CPlugin::SetDirectory(const char *aszDir, int opMode)
{
  UString aDir = MultiByteToUnicodeString(aszDir, CP_OEMCP);
  if (aDir == L"\\")
  {
    _folder.Release();
    m_ArchiveHandler->BindToRootFolder(&_folder);  
  }
  else if (aDir == L"..")
  {
    CComPtr<IFolderFolder> newFolder;
    _folder->BindToParentFolder(&newFolder);  
    if (newFolder == NULL)
      throw 40312;
    _folder = newFolder;
  }
  else if (aDir.IsEmpty())
    EnterToDirectory(aDir);
  else
  {
    if (aDir[0] == L'\\')
    {
      _folder.Release();
      m_ArchiveHandler->BindToRootFolder(&_folder);  
      aDir = aDir.Mid(1);
    }
    UStringVector pathParts;
    SplitPathToParts(aDir, pathParts);
    for(int i = 0; i < pathParts.Size(); i++)
      EnterToDirectory(pathParts[i]);
  }
  m_CurrentDir.Empty();
  UStringVector pathParts;
  GetPathParts(pathParts);
  for (int i = 0; i < pathParts.Size(); i++)
  {
    m_CurrentDir += CSysString('\\');
    m_CurrentDir += UnicodeStringToMultiByte(pathParts[i], CP_OEMCP);
  }
  return TRUE;
}

void CPlugin::GetPathParts(UStringVector &pathParts)
{
  pathParts.Clear();
  CComPtr<IFolderFolder> aFolderItem = _folder;
  while (true)
  {
    CComPtr<IFolderFolder> newFolder;
    aFolderItem->BindToParentFolder(&newFolder);  
    if (newFolder == NULL)
      break;
    CComBSTR name;
    aFolderItem->GetName(&name);
    pathParts.Insert(0, (const wchar_t *)name);
    aFolderItem = newFolder;
  }
}

static char *kPluginFormatName = "7-ZIP";


struct CPROPIDToName
{
  PROPID PropID;
  int PluginID;
};

static CPROPIDToName kPROPIDToName[] =  
{
  { kpidName, NMessageID::kName },
  { kpidIsFolder, NMessageID::kIsFolder }, 
  { kpidSize, NMessageID::kSize },
  { kpidPackedSize, NMessageID::kPackedSize },
  { kpidAttributes, NMessageID::kAttributes },
  { kpidCreationTime, NMessageID::kCreationTime },
  { kpidLastAccessTime, NMessageID::kLastAccessTime },
  { kpidLastWriteTime, NMessageID::kLastWriteTime },
  { kpidSolid, NMessageID::kSolid },
  { kpidComment, NMessageID::kComment },
  { kpidEncrypted, NMessageID::kEncrypted },
  { kpidSplitBefore, NMessageID::kSplitBefore },
  { kpidSplitAfter, NMessageID::kSplitAfter },
  { kpidDictionarySize, NMessageID::kDictionarySize },
  { kpidCRC, NMessageID::kCRC },
  { kpidMethod, NMessageID::kMethod },
  { kpidHostOS, NMessageID::kHostOS }
};

static const kNumPROPIDToName = sizeof(kPROPIDToName) /  sizeof(kPROPIDToName[0]);

static int FindPropertyToName(PROPID propID)
{
  for(int i = 0; i < kNumPROPIDToName; i++)
    if(kPROPIDToName[i].PropID == propID)
      return i;
  return -1;
}

/*
struct CPropertyIDInfo
{
  PROPID PropID;
  const char *FarID;
  int Width;
  // char CharID;
};

static CPropertyIDInfo kPropertyIDInfos[] =  
{
  { kpidName, "N", 0},
  { kpidSize, "S", 8},
  { kpidPackedSize, "P", 8},
  { kpidAttributes, "A", 0},
  { kpidCreationTime, "DC", 14},
  { kpidLastAccessTime, "DA", 14},
  { kpidLastWriteTime, "DM", 14},
  
  { kpidSolid, NULL, 0, 'S'},
  { kpidEncrypted, NULL, 0, 'P'}

  { kpidDictionarySize, IDS_PROPERTY_DICTIONARY_SIZE },
  { kpidSplitBefore, NULL, 'B'},
  { kpidSplitAfter, NULL, 'A'},
  { kpidComment, , NULL, 'C'},
  { kpidCRC, IDS_PROPERTY_CRC }
  // { kpidType, L"Type" }
};

static const kNumPropertyIDInfos = sizeof(kPropertyIDInfos) /  
    sizeof(kPropertyIDInfos[0]);

static int FindPropertyInfo(PROPID propID)
{
  for(int i = 0; i < kNumPropertyIDInfos; i++)
    if(kPropertyIDInfos[i].PropID == propID)
      return i;
  return -1;
}
*/

// char *g_Titles[] = { "a", "f", "v" };
static void SmartAddToString(AString &aDestString, const char *aSrcString)
{
  if (!aDestString.IsEmpty())
    aDestString += ',';
  aDestString += aSrcString;
}

/*
void CPlugin::AddColumn(PROPID propID)
{
  int index = FindPropertyInfo(propID);
  if (index >= 0)
  {
    for(int i = 0; i < m_ProxyHandler->m_InternalProperties.Size(); i++)
    {
      const CArchiveItemProperty &aHandlerProperty = m_ProxyHandler->m_InternalProperties[i];
      if (aHandlerProperty.ID == propID)
        break;
    }
    if (i == m_ProxyHandler->m_InternalProperties.Size())
      return;

    const CPropertyIDInfo &aPropertyIDInfo = kPropertyIDInfos[index];
    SmartAddToString(PanelModeColumnTypes, aPropertyIDInfo.FarID);
    char aTmp[32];
    itoa(aPropertyIDInfo.Width, aTmp, 10);
    SmartAddToString(PanelModeColumnWidths, aTmp);
    return;
  }
}
*/

void CPlugin::GetOpenPluginInfo(struct OpenPluginInfo *info)
{
  info->StructSize = sizeof(*info);
  info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS| OPIF_USEHIGHLIGHTING|
              OPIF_ADDDOTS | OPIF_COMPAREFATTIME;

  strcpy(m_FileNameBuffer, m_FileName);
  info->HostFile = m_FileNameBuffer; // test it it is not static
  
  strcpy(m_CurrentDirBuffer, m_CurrentDir);
  info->CurDir = m_CurrentDirBuffer;

  info->Format = kPluginFormatName;

  CSysString name;
  {
    CSysString fullName;
    int index;
    NFile::NDirectory::MyGetFullPathName(m_FileName, fullName, index);
    name = fullName.Mid(index);
  }

  m_PannelTitle = m_ArchiverInfo.Name + ':' + name;
  if(!m_CurrentDir.IsEmpty())
  {
    // m_PannelTitle += '\\';
    m_PannelTitle += m_CurrentDir;
  }
 
  strcpy(m_PannelTitleBuffer, m_PannelTitle);
  info->PanelTitle = m_PannelTitleBuffer;

  memset(m_InfoLines,0,sizeof(m_InfoLines));
  strcpy(m_InfoLines[0].Text,"");
  m_InfoLines[0].Separator = TRUE;

  strcpy(m_InfoLines[1].Text, g_StartupInfo.GetMsgString(NMessageID::kArchiveType));
  strcpy(m_InfoLines[1].Data, m_ArchiverInfo.Name);
  //m_InfoLines[1].Separator = 0;

  info->InfoLines = m_InfoLines;
  info->InfoLinesNumber = sizeof(m_InfoLines) / sizeof(m_InfoLines[0]);

  
  info->DescrFiles = NULL;
  info->DescrFilesNumber = 0;

  PanelModeColumnTypes.Empty();
  PanelModeColumnWidths.Empty();

  /*
  AddColumn(kpidName);
  AddColumn(kpidSize);
  AddColumn(kpidPackedSize);
  AddColumn(kpidLastWriteTime);
  AddColumn(kpidCreationTime);
  AddColumn(kpidLastAccessTime);
  AddColumn(kpidAttributes);
  
  PanelMode.ColumnTypes = (char *)(const char *)PanelModeColumnTypes;
  PanelMode.ColumnWidths = (char *)(const char *)PanelModeColumnWidths;
  PanelMode.ColumnTitles = NULL;
  PanelMode.FullScreen = TRUE;
  PanelMode.DetailedStatus = FALSE;
  PanelMode.AlignExtensions = FALSE;
  PanelMode.CaseConversion = FALSE;
  PanelMode.StatusColumnTypes = "N";
  PanelMode.StatusColumnWidths = "0";
  PanelMode.Reserved[0] = 0;
  PanelMode.Reserved[1] = 0;

  info->PanelModesArray = &PanelMode;
  info->PanelModesNumber = 1;
  */

  info->PanelModesArray = NULL;
  info->PanelModesNumber = 0;

  info->StartPanelMode = 0;
  info->StartSortMode = 0;
  info->KeyBar = NULL;
  info->ShortcutData = NULL;
}

CSysString ConvertPropertyToString2(const PROPVARIANT &propVariant, PROPID propID)
{
  if (propVariant.vt == VT_BSTR)
    return GetSystemString(propVariant.bstrVal, CP_OEMCP);
  if (propVariant.vt != VT_BOOL)
    return ConvertPropertyToString(propVariant, propID);
  int messageID = VARIANT_BOOLToBool(propVariant.boolVal) ? 
      NMessageID::kYes : NMessageID::kNo;
  return g_StartupInfo.GetMsgString(messageID);
}

struct CArchiveItemProperty
{
  AString Name;
  PROPID ID;
  VARTYPE Type;
};

HRESULT CPlugin::ShowAttributesWindow()
{
  PluginPanelItem pluginPanelItem;
  if (!g_StartupInfo.ControlGetActivePanelCurrentItemInfo(pluginPanelItem))
    return S_FALSE;
  if (strcmp(pluginPanelItem.FindData.cFileName, "..") == 0 && 
        NFile::NFind::NAttributes::IsDirectory(pluginPanelItem.FindData.dwFileAttributes))
    return S_FALSE;
  int itemIndex = pluginPanelItem.UserData;

  CComPtr<IEnumSTATPROPSTG> enumProperty;
  RETURN_IF_NOT_S_OK(m_ArchiveHandler->EnumProperties(&enumProperty));
  
  STATPROPSTG srcProperty;
  CObjectVector<CArchiveItemProperty> m_Properties;
  while (enumProperty->Next(1, &srcProperty, NULL) == S_OK)
  {
    CArchiveItemProperty destProperty;
    destProperty.Type = srcProperty.vt;
    destProperty.ID = srcProperty.propid;
    if (destProperty.ID  == kpidPath)
      destProperty.ID  = kpidName;
    UINT propID = srcProperty.propid;
    AString propName;
    {
      if (srcProperty.lpwstrName != NULL)
        destProperty.Name = UnicodeStringToMultiByte(srcProperty.lpwstrName, CP_OEMCP);
      else
        destProperty.Name = "Error";
    }
    if (srcProperty.lpwstrName != NULL)
      CoTaskMemFree(srcProperty.lpwstrName);
    m_Properties.Add(destProperty);
  }

  /*
  LPCITEMIDLIST aProperties;
  if (index < m_FolderItem->m_DirSubItems.Size())
  {
    const CArchiveFolderItem &anItem = m_FolderItem->m_DirSubItems[index];
    aProperties = anItem.m_Properties;
  }
  else
  {
    const CArchiveFolderFileItem &anItem = 
        m_FolderItem->m_FileSubItems[index - m_FolderItem->m_DirSubItems.Size()];
    aProperties = anItem.m_Properties;
  }
  */

  const kPathIndex = 2;
  const kOkButtonIndex = 4;
  int size = 2;
  CRecordVector<CInitDialogItem> anInitDialogItems;
  
  int xSize = 70;
  CInitDialogItem initDialogItem = 
  { DI_DOUBLEBOX, 3, 1, xSize - 4, size - 2, false, false, 0, false, NMessageID::kProperties, NULL, NULL };
  anInitDialogItems.Add(initDialogItem);
  AStringVector aValues;

  for (int i = 0; i < m_Properties.Size(); i++)
  {
    const CArchiveItemProperty &aProperty = m_Properties[i];

    CInitDialogItem initDialogItem = 
      { DI_TEXT, 5, 3 + i, 0, 0, false, false, 0, false, 0, NULL, NULL };
    int index = FindPropertyToName(aProperty.ID);
    if (index < 0)
    {
      initDialogItem.DataMessageId = -1;
      initDialogItem.DataString = aProperty.Name;
    }
    else
      initDialogItem.DataMessageId = kPROPIDToName[index].PluginID;
    anInitDialogItems.Add(initDialogItem);
    
    NCOM::CPropVariant propVariant;
    RETURN_IF_NOT_S_OK(_folder->GetProperty(itemIndex, 
        aProperty.ID, &propVariant));
    CSysString aString = ConvertPropertyToString2(propVariant, aProperty.ID);
    aValues.Add(aString);
    
    {
      CInitDialogItem initDialogItem = 
      { DI_TEXT, 30, 3 + i, 0, 0, false, false, 0, false, -1, NULL, NULL };
      anInitDialogItems.Add(initDialogItem);
    }
  }

  int numLines = aValues.Size();
  for(i = 0; i < numLines; i++)
  {
    CInitDialogItem &initDialogItem = anInitDialogItems[1 + i * 2 + 1];
    initDialogItem.DataString = aValues[i];
  }
  
  int numDialogItems = anInitDialogItems.Size();
  
  CRecordVector<FarDialogItem> dialogItems;
  dialogItems.Reserve(numDialogItems);
  for(i = 0; i < numDialogItems; i++)
    dialogItems.Add(FarDialogItem());
  g_StartupInfo.InitDialogItems(anInitDialogItems.GetPointer(), 
    dialogItems.GetPointer(), numDialogItems);
  
  int maxLen = 0;
  for (i = 0; i < numLines; i++)
  {
    FarDialogItem &dialogItem = dialogItems[1 + i * 2];
    int len = strlen(dialogItem.Data);
    if (len > maxLen)
      maxLen = len;
  }
  int maxLen2 = 0;
  const kSpace = 10;
  for (i = 0; i < numLines; i++)
  {
    FarDialogItem &dialogItem = dialogItems[1 + i * 2 + 1];
    int len = strlen(dialogItem.Data);
    if (len > maxLen2)
      maxLen2 = len;
    dialogItem.X1 = maxLen + kSpace;
  }
  size = numLines + 6;
  xSize = maxLen + kSpace + maxLen2 + 5;
  FarDialogItem &aFirstDialogItem = dialogItems.Front();
  aFirstDialogItem.Y2 = size - 2;
  aFirstDialogItem.X2 = xSize - 4;
  
  int askCode = g_StartupInfo.ShowDialog(xSize, size, NULL, dialogItems.GetPointer(), numDialogItems);
  return S_OK;
}

int CPlugin::ProcessKey(int aKey, unsigned int controlState)
{
  if (controlState == PKF_CONTROL && aKey == 'A')
  {
    HRESULT result = ShowAttributesWindow();
    if (result == S_OK)
      return TRUE;
    if (result == S_FALSE)
      return FALSE;
    throw "Error";
  }
  if ((controlState & PKF_ALT) != 0 && aKey == VK_F6)
  {
    CSysString folderPath;
    if (!NFile::NDirectory::GetOnlyDirPrefix(m_FileName, folderPath))
      return FALSE;
    PanelInfo panelInfo;
    g_StartupInfo.ControlGetActivePanelInfo(panelInfo);
    GetFilesReal(panelInfo.SelectedItems,
        panelInfo.SelectedItemsNumber, FALSE, 
        (char *)(const char *)folderPath, OPM_SILENT, true);  
    g_StartupInfo.Control(this, FCTL_UPDATEPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_REDRAWPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_UPDATEANOTHERPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_REDRAWANOTHERPANEL, NULL);
    return TRUE;
  }

  return FALSE;
}
