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

CPlugin::CPlugin(const CSysString &aFileName, 
    const UString &aDefaultName, 
    IArchiveHandler100 *anArchiveHandler,
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo):
  m_ArchiveHandler(anArchiveHandler),
  m_FileName(aFileName),
  m_DefaultName(aDefaultName),
  m_ArchiverInfo(anArchiverInfo)
{
  if (!NFile::NFind::FindFile(m_FileName, m_FileInfo))
    throw "error";
  anArchiveHandler->BindToRootFolder(&_folder);
}

CPlugin::~CPlugin()
{
}

static void MyGetFileTime(IFolderFolder *anArchiveFolder, UINT32 anItemIndex,
    PROPID aPropID, FILETIME &aFileTime)
{
  NCOM::CPropVariant aPropVariant;
  if (anArchiveFolder->GetProperty(anItemIndex, aPropID, &aPropVariant) != S_OK)
    throw 271932;
  if (aPropVariant.vt == VT_EMPTY)
  {
    aFileTime.dwHighDateTime = 0;
    aFileTime.dwLowDateTime = 0;
  }
  else 
  {
    if (aPropVariant.vt != VT_FILETIME)
      throw 4191730;
    aFileTime = aPropVariant.filetime;
  }
}
  
void CPlugin::ReadPluginPanelItem(PluginPanelItem &aPanelItem, UINT32 anItemIndex)
{
  NCOM::CPropVariant aPropVariant;
  if (_folder->GetProperty(anItemIndex, kpidName, &aPropVariant) != S_OK)
    throw 271932;

  if (aPropVariant.vt != VT_BSTR)
    throw 272340;

  CSysString anOemString = UnicodeStringToMultiByte(aPropVariant.bstrVal, CP_OEMCP);
  strcpy(aPanelItem.FindData.cFileName, anOemString);
  aPanelItem.FindData.cAlternateFileName[0] = 0;

  if (_folder->GetProperty(anItemIndex, kpidAttributes, &aPropVariant) != S_OK)
    throw 271932;
  if (aPropVariant.vt == VT_UI4)
    aPanelItem.FindData.dwFileAttributes  = aPropVariant.ulVal;
  else if (aPropVariant.vt == VT_EMPTY)
    aPanelItem.FindData.dwFileAttributes = m_FileInfo.Attributes;
  else
    throw 21631;

  if (_folder->GetProperty(anItemIndex, kpidIsFolder, &aPropVariant) != S_OK)
    throw 271932;
  if (aPropVariant.vt == VT_BOOL)
  {
    if (VARIANT_BOOLToBool(aPropVariant.boolVal))
      aPanelItem.FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  }
  else if (aPropVariant.vt != VT_EMPTY)
    throw 21632;

  if (_folder->GetProperty(anItemIndex, kpidSize, &aPropVariant) != S_OK)
    throw 271932;
  UINT64 aLength;
  if (aPropVariant.vt == VT_EMPTY)
    aLength = 0;
  else
    aLength = ::ConvertPropVariantToUINT64(aPropVariant);
  aPanelItem.FindData.nFileSizeLow = UINT32(aLength);
  aPanelItem.FindData.nFileSizeHigh = UINT32(aLength >> 32);

  MyGetFileTime(_folder, anItemIndex, kpidCreationTime, aPanelItem.FindData.ftCreationTime);
  MyGetFileTime(_folder, anItemIndex, kpidLastAccessTime, aPanelItem.FindData.ftLastAccessTime);
  MyGetFileTime(_folder, anItemIndex, kpidLastWriteTime, aPanelItem.FindData.ftLastWriteTime);

  if (aPanelItem.FindData.ftLastWriteTime.dwHighDateTime == 0 && 
      aPanelItem.FindData.ftLastWriteTime.dwLowDateTime == 0)
    aPanelItem.FindData.ftLastWriteTime = m_FileInfo.LastWriteTime;

  if (_folder->GetProperty(anItemIndex, kpidPackedSize, &aPropVariant) != S_OK)
    throw 271932;
  if (aPropVariant.vt == VT_EMPTY)
    aLength = 0;
  else
    aLength = ::ConvertPropVariantToUINT64(aPropVariant);
  aPanelItem.PackSize = UINT32(aLength);
  aPanelItem.PackSizeHigh = UINT32(aLength >> 32);

  aPanelItem.Flags = 0;
  aPanelItem.NumberOfLinks = 0;

  aPanelItem.Description = NULL;
  aPanelItem.Owner = NULL;
  aPanelItem.CustomColumnData = NULL;
  aPanelItem.CustomColumnNumber = 0;

  aPanelItem.Reserved[0] = 0;
  aPanelItem.Reserved[1] = 0;
  aPanelItem.Reserved[2] = 0;
  

}

int CPlugin::GetFindData(PluginPanelItem **aPanelItems, 
    int *anItemsNumber, int anOpMode)
{
  // CScreenRestorer aScreenRestorer;
  if ((anOpMode & OPM_SILENT) == 0 && (anOpMode & OPM_FIND ) == 0)
  {
    /*
    aScreenRestorer.Save();
    const char *aMsgItems[]=
    {
      g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kReadingList)
    };
    g_StartupInfo.ShowMessage(0, NULL, aMsgItems,
      sizeof(aMsgItems) / sizeof(aMsgItems[0]), 0);
    */
  }

  UINT32 aNumItems;
  _folder->GetNumberOfItems(&aNumItems);
  *aPanelItems = new PluginPanelItem[aNumItems];
  try
  {
    for(int i = 0; i < aNumItems; i++)
    {
      PluginPanelItem &aPanelItem = (*aPanelItems)[i];
      ReadPluginPanelItem(aPanelItem, i);
      aPanelItem.UserData = i;
    }
  }
  catch(...)
  {
    delete [](*aPanelItems);
    throw;
  }
  *anItemsNumber = aNumItems;
  return(TRUE);
}

void CPlugin::FreeFindData(struct PluginPanelItem *aPanelItems,
    int anItemsNumber)
{
  for(int i = 0; i < anItemsNumber; i++)
    if(aPanelItems[i].Description != NULL)
      delete []aPanelItems[i].Description;
  delete []aPanelItems;
}


void CPlugin::EnterToDirectory(const UString &aDirName)
{
  CComPtr<IFolderFolder> aNewFolder;
  _folder->BindToFolder(aDirName, &aNewFolder);
  if(aNewFolder == NULL)
    if (aDirName.IsEmpty())
      return;
    else
      throw 40325;
  _folder = aNewFolder;
}

int CPlugin::SetDirectory(const char *aszDir, int anOpMode)
{
  UString aDir = MultiByteToUnicodeString(aszDir, CP_OEMCP);
  if (aDir == L"\\")
  {
    _folder.Release();
    m_ArchiveHandler->BindToRootFolder(&_folder);  
  }
  else if (aDir == L"..")
  {
    CComPtr<IFolderFolder> aNewFolder;
    _folder->BindToParentFolder(&aNewFolder);  
    if (aNewFolder == NULL)
      throw 40312;
    _folder = aNewFolder;
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
    UStringVector aPathParts;
    SplitPathToParts(aDir, aPathParts);
    for(int i = 0; i < aPathParts.Size(); i++)
      EnterToDirectory(aPathParts[i]);
  }
  m_CurrentDir.Empty();
  UStringVector aPathParts;
  GetPathParts(aPathParts);
  for (int i = 0; i < aPathParts.Size(); i++)
  {
    m_CurrentDir += CSysString('\\');
    m_CurrentDir += UnicodeStringToMultiByte(aPathParts[i], CP_OEMCP);
  }
  return TRUE;
}

void CPlugin::GetPathParts(UStringVector &aPathParts)
{
  aPathParts.Clear();
  CComPtr<IFolderFolder> aFolderItem = _folder;
  while (true)
  {
    CComPtr<IFolderFolder> aNewFolder;
    aFolderItem->BindToParentFolder(&aNewFolder);  
    if (aNewFolder == NULL)
      break;
    CComBSTR aName;
    aFolderItem->GetName(&aName);
    aPathParts.Insert(0, (const wchar_t *)aName);
    aFolderItem = aNewFolder;
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

static int FindPropertyToName(PROPID aPropID)
{
  for(int i = 0; i < kNumPROPIDToName; i++)
    if(kPROPIDToName[i].PropID == aPropID)
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

static int FindPropertyInfo(PROPID aPropID)
{
  for(int i = 0; i < kNumPropertyIDInfos; i++)
    if(kPropertyIDInfos[i].PropID == aPropID)
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
void CPlugin::AddColumn(PROPID aPropID)
{
  int anIndex = FindPropertyInfo(aPropID);
  if (anIndex >= 0)
  {
    for(int i = 0; i < m_ProxyHandler->m_InternalProperties.Size(); i++)
    {
      const CArchiveItemProperty &aHandlerProperty = m_ProxyHandler->m_InternalProperties[i];
      if (aHandlerProperty.ID == aPropID)
        break;
    }
    if (i == m_ProxyHandler->m_InternalProperties.Size())
      return;

    const CPropertyIDInfo &aPropertyIDInfo = kPropertyIDInfos[anIndex];
    SmartAddToString(PanelModeColumnTypes, aPropertyIDInfo.FarID);
    char aTmp[32];
    itoa(aPropertyIDInfo.Width, aTmp, 10);
    SmartAddToString(PanelModeColumnWidths, aTmp);
    return;
  }
}
*/

void CPlugin::GetOpenPluginInfo(struct OpenPluginInfo *anInfo)
{
  anInfo->StructSize = sizeof(*anInfo);
  anInfo->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS| OPIF_USEHIGHLIGHTING|
              OPIF_ADDDOTS | OPIF_COMPAREFATTIME;

  strcpy(m_FileNameBuffer, m_FileName);
  anInfo->HostFile = m_FileNameBuffer; // test it it is not static
  
  strcpy(m_CurrentDirBuffer, m_CurrentDir);
  anInfo->CurDir = m_CurrentDirBuffer;

  anInfo->Format = kPluginFormatName;

  CSysString aName;
  {
    CSysString aFullName;
    int anIndex;
    NFile::NDirectory::MyGetFullPathName(m_FileName, aFullName, anIndex);
    aName = aFullName.Mid(anIndex);
  }

  m_PannelTitle = m_ArchiverInfo.Name + ':' + aName;
  if(!m_CurrentDir.IsEmpty())
  {
    // m_PannelTitle += '\\';
    m_PannelTitle += m_CurrentDir;
  }
 
  strcpy(m_PannelTitleBuffer, m_PannelTitle);
  anInfo->PanelTitle = m_PannelTitleBuffer;

  memset(m_InfoLines,0,sizeof(m_InfoLines));
  strcpy(m_InfoLines[0].Text,"");
  m_InfoLines[0].Separator = TRUE;

  strcpy(m_InfoLines[1].Text, g_StartupInfo.GetMsgString(NMessageID::kArchiveType));
  strcpy(m_InfoLines[1].Data, m_ArchiverInfo.Name);
  //m_InfoLines[1].Separator = 0;

  anInfo->InfoLines = m_InfoLines;
  anInfo->InfoLinesNumber = sizeof(m_InfoLines) / sizeof(m_InfoLines[0]);

  
  anInfo->DescrFiles = NULL;
  anInfo->DescrFilesNumber = 0;

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

  anInfo->PanelModesArray = &PanelMode;
  anInfo->PanelModesNumber = 1;
  */

  anInfo->PanelModesArray = NULL;
  anInfo->PanelModesNumber = 0;

  anInfo->StartPanelMode = 0;
  anInfo->StartSortMode = 0;
  anInfo->KeyBar = NULL;
  anInfo->ShortcutData = NULL;
}

CSysString ConvertPropertyToString2(const PROPVARIANT &aPropVariant, PROPID aPropID)
{
  if (aPropVariant.vt == VT_BSTR)
    return GetSystemString(aPropVariant.bstrVal, CP_OEMCP);
  if (aPropVariant.vt != VT_BOOL)
    return ConvertPropertyToString(aPropVariant, aPropID);
  int aMessageID = VARIANT_BOOLToBool(aPropVariant.boolVal) ? 
      NMessageID::kYes : NMessageID::kNo;
  return g_StartupInfo.GetMsgString(aMessageID);
}

struct CArchiveItemProperty
{
  AString Name;
  PROPID ID;
  VARTYPE Type;
};

HRESULT CPlugin::ShowAttributesWindow()
{
  PluginPanelItem aPluginPanelItem;
  if (!g_StartupInfo.ControlGetActivePanelCurrentItemInfo(aPluginPanelItem))
    return S_FALSE;
  if (strcmp(aPluginPanelItem.FindData.cFileName, "..") == 0 && 
        NFile::NFind::NAttributes::IsDirectory(aPluginPanelItem.FindData.dwFileAttributes))
    return S_FALSE;
  int anItemIndex = aPluginPanelItem.UserData;

  CComPtr<IEnumSTATPROPSTG> anEnumProperty;
  RETURN_IF_NOT_S_OK(m_ArchiveHandler->EnumProperties(&anEnumProperty));
  
  STATPROPSTG aSrcProperty;
  CObjectVector<CArchiveItemProperty> m_Properties;
  while (anEnumProperty->Next(1, &aSrcProperty, NULL) == S_OK)
  {
    CArchiveItemProperty aDestProperty;
    aDestProperty.Type = aSrcProperty.vt;
    aDestProperty.ID = aSrcProperty.propid;
    if (aDestProperty.ID  == kpidPath)
      aDestProperty.ID  = kpidName;
    UINT aPropID = aSrcProperty.propid;
    AString aPropName;
    {
      if (aSrcProperty.lpwstrName != NULL)
        aDestProperty.Name = UnicodeStringToMultiByte(aSrcProperty.lpwstrName, CP_OEMCP);
      else
        aDestProperty.Name = "Error";
    }
    if (aSrcProperty.lpwstrName != NULL)
      CoTaskMemFree(aSrcProperty.lpwstrName);
    m_Properties.Add(aDestProperty);
  }

  /*
  LPCITEMIDLIST aProperties;
  if (anIndex < m_FolderItem->m_DirSubItems.Size())
  {
    const CArchiveFolderItem &anItem = m_FolderItem->m_DirSubItems[anIndex];
    aProperties = anItem.m_Properties;
  }
  else
  {
    const CArchiveFolderFileItem &anItem = 
        m_FolderItem->m_FileSubItems[anIndex - m_FolderItem->m_DirSubItems.Size()];
    aProperties = anItem.m_Properties;
  }
  */

  const kPathIndex = 2;
  const kOkButtonIndex = 4;
  int anYSize = 2;
  CRecordVector<CInitDialogItem> anInitDialogItems;
  
  int aXSize = 70;
  CInitDialogItem anInitDialogItem = 
  { DI_DOUBLEBOX, 3, 1, aXSize - 4, anYSize - 2, false, false, 0, false, NMessageID::kProperties, NULL, NULL };
  anInitDialogItems.Add(anInitDialogItem);
  AStringVector aValues;

  for (int i = 0; i < m_Properties.Size(); i++)
  {
    const CArchiveItemProperty &aProperty = m_Properties[i];

    CInitDialogItem anInitDialogItem = 
      { DI_TEXT, 5, 3 + i, 0, 0, false, false, 0, false, 0, NULL, NULL };
    int anIndex = FindPropertyToName(aProperty.ID);
    if (anIndex < 0)
    {
      anInitDialogItem.DataMessageId = -1;
      anInitDialogItem.DataString = aProperty.Name;
    }
    else
      anInitDialogItem.DataMessageId = kPROPIDToName[anIndex].PluginID;
    anInitDialogItems.Add(anInitDialogItem);
    
    NCOM::CPropVariant aPropVariant;
    RETURN_IF_NOT_S_OK(_folder->GetProperty(anItemIndex, 
        aProperty.ID, &aPropVariant));
    CSysString aString = ConvertPropertyToString2(aPropVariant, aProperty.ID);
    aValues.Add(aString);
    
    {
      CInitDialogItem anInitDialogItem = 
      { DI_TEXT, 30, 3 + i, 0, 0, false, false, 0, false, -1, NULL, NULL };
      anInitDialogItems.Add(anInitDialogItem);
    }
  }

  int aNumLines = aValues.Size();
  for(i = 0; i < aNumLines; i++)
  {
    CInitDialogItem &anInitDialogItem = anInitDialogItems[1 + i * 2 + 1];
    anInitDialogItem.DataString = aValues[i];
  }
  
  int aNumDialogItems = anInitDialogItems.Size();
  
  CRecordVector<FarDialogItem> aDialogItems;
  aDialogItems.Reserve(aNumDialogItems);
  for(i = 0; i < aNumDialogItems; i++)
    aDialogItems.Add(FarDialogItem());
  g_StartupInfo.InitDialogItems(anInitDialogItems.GetPointer(), 
    aDialogItems.GetPointer(), aNumDialogItems);
  
  int aMaxLen = 0;
  for (i = 0; i < aNumLines; i++)
  {
    FarDialogItem &aDialogItem = aDialogItems[1 + i * 2];
    int aLen = strlen(aDialogItem.Data);
    if (aLen > aMaxLen)
      aMaxLen = aLen;
  }
  int aMaxLen2 = 0;
  const kSpace = 10;
  for (i = 0; i < aNumLines; i++)
  {
    FarDialogItem &aDialogItem = aDialogItems[1 + i * 2 + 1];
    int aLen = strlen(aDialogItem.Data);
    if (aLen > aMaxLen2)
      aMaxLen2 = aLen;
    aDialogItem.X1 = aMaxLen + kSpace;
  }
  anYSize = aNumLines + 6;
  aXSize = aMaxLen + kSpace + aMaxLen2 + 5;
  FarDialogItem &aFirstDialogItem = aDialogItems.Front();
  aFirstDialogItem.Y2 = anYSize - 2;
  aFirstDialogItem.X2 = aXSize - 4;
  
  int anAskCode = g_StartupInfo.ShowDialog(aXSize, anYSize, NULL, aDialogItems.GetPointer(), aNumDialogItems);
  return S_OK;
}

int CPlugin::ProcessKey(int aKey, unsigned int aControlState)
{
  if (aControlState == PKF_CONTROL && aKey == 'A')
  {
    HRESULT aResult = ShowAttributesWindow();
    if (aResult == S_OK)
      return TRUE;
    if (aResult == S_FALSE)
      return FALSE;
    throw "Error";
  }
  if ((aControlState & PKF_ALT) != 0 && aKey == VK_F6)
  {
    CSysString aFolderPath;
    if (!NFile::NDirectory::GetOnlyDirPrefix(m_FileName, aFolderPath))
      return FALSE;
    PanelInfo aPanelInfo;
    g_StartupInfo.ControlGetActivePanelInfo(aPanelInfo);
    GetFilesReal(aPanelInfo.SelectedItems,
        aPanelInfo.SelectedItemsNumber, FALSE, 
        (char *)(const char *)aFolderPath, OPM_SILENT, true);  
    g_StartupInfo.Control(this, FCTL_UPDATEPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_REDRAWPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_UPDATEANOTHERPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_REDRAWANOTHERPANEL, NULL);
    return TRUE;
  }

  return FALSE;
}
