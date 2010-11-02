// Plugin.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "Common/Wildcard.h"

#include "Windows/FileDir.h"
#include "Windows/PropVariantConversions.h"

#include "../Common/PropIDUtils.h"

#include "FarUtils.h"
#include "Messages.h"
#include "Plugin.h"

using namespace NWindows;
using namespace NFar;

CPlugin::CPlugin(const UString &fileName, IInFolderArchive *archiveHandler, UString archiveTypeName):
    m_ArchiveHandler(archiveHandler),
    m_FileName(fileName),
    _archiveTypeName(archiveTypeName)
{
  if (!m_FileInfo.Find(m_FileName))
    throw "error";
  archiveHandler->BindToRootFolder(&_folder);
}

CPlugin::~CPlugin() {}

static void MyGetFileTime(IFolderFolder *anArchiveFolder, UInt32 itemIndex,
    PROPID propID, FILETIME &fileTime)
{
  NCOM::CPropVariant prop;
  if (anArchiveFolder->GetProperty(itemIndex, propID, &prop) != S_OK)
    throw 271932;
  if (prop.vt == VT_EMPTY)
  {
    fileTime.dwHighDateTime = 0;
    fileTime.dwLowDateTime = 0;
  }
  else
  {
    if (prop.vt != VT_FILETIME)
      throw 4191730;
    fileTime = prop.filetime;
  }
}

#define kDotsReplaceString "[[..]]"
#define kDotsReplaceStringU L"[[..]]"
  
static void CopyStrLimited(char *dest, const AString &src, int len)
{
  len--;
  if (src.Length() < len)
    len = src.Length();
  memcpy(dest, src, sizeof(dest[0]) * len);
  dest[len] = 0;
}

#define COPY_STR_LIMITED(dest, src) CopyStrLimited(dest, src, sizeof(dest) / sizeof(dest[0]))

void CPlugin::ReadPluginPanelItem(PluginPanelItem &panelItem, UInt32 itemIndex)
{
  NCOM::CPropVariant prop;
  if (_folder->GetProperty(itemIndex, kpidName, &prop) != S_OK)
    throw 271932;

  if (prop.vt != VT_BSTR)
    throw 272340;

  AString oemString = UnicodeStringToMultiByte(prop.bstrVal, CP_OEMCP);
  if (oemString == "..")
    oemString = kDotsReplaceString;

  COPY_STR_LIMITED(panelItem.FindData.cFileName, oemString);
  panelItem.FindData.cAlternateFileName[0] = 0;

  if (_folder->GetProperty(itemIndex, kpidAttrib, &prop) != S_OK)
    throw 271932;
  if (prop.vt == VT_UI4)
    panelItem.FindData.dwFileAttributes  = prop.ulVal;
  else if (prop.vt == VT_EMPTY)
    panelItem.FindData.dwFileAttributes = m_FileInfo.Attrib;
  else
    throw 21631;

  if (_folder->GetProperty(itemIndex, kpidIsDir, &prop) != S_OK)
    throw 271932;
  if (prop.vt == VT_BOOL)
  {
    if (VARIANT_BOOLToBool(prop.boolVal))
      panelItem.FindData.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  }
  else if (prop.vt != VT_EMPTY)
    throw 21632;

  if (_folder->GetProperty(itemIndex, kpidSize, &prop) != S_OK)
    throw 271932;
  UInt64 length;
  if (prop.vt == VT_EMPTY)
    length = 0;
  else
    length = ::ConvertPropVariantToUInt64(prop);
  panelItem.FindData.nFileSizeLow = (UInt32)length;
  panelItem.FindData.nFileSizeHigh = (UInt32)(length >> 32);

  MyGetFileTime(_folder, itemIndex, kpidCTime, panelItem.FindData.ftCreationTime);
  MyGetFileTime(_folder, itemIndex, kpidATime, panelItem.FindData.ftLastAccessTime);
  MyGetFileTime(_folder, itemIndex, kpidMTime, panelItem.FindData.ftLastWriteTime);

  if (panelItem.FindData.ftLastWriteTime.dwHighDateTime == 0 &&
      panelItem.FindData.ftLastWriteTime.dwLowDateTime == 0)
    panelItem.FindData.ftLastWriteTime = m_FileInfo.MTime;

  if (_folder->GetProperty(itemIndex, kpidPackSize, &prop) != S_OK)
    throw 271932;
  if (prop.vt == VT_EMPTY)
    length = 0;
  else
    length = ::ConvertPropVariantToUInt64(prop);
  panelItem.PackSize = UInt32(length);
  panelItem.PackSizeHigh = UInt32(length >> 32);

  panelItem.Flags = 0;
  panelItem.NumberOfLinks = 0;

  panelItem.Description = NULL;
  panelItem.Owner = NULL;
  panelItem.CustomColumnData = NULL;
  panelItem.CustomColumnNumber = 0;

  panelItem.CRC32 = 0;
  panelItem.Reserved[0] = 0;
  panelItem.Reserved[1] = 0;
}

int CPlugin::GetFindData(PluginPanelItem **panelItems, int *itemsNumber, int opMode)
{
  // CScreenRestorer screenRestorer;
  if ((opMode & OPM_SILENT) == 0 && (opMode & OPM_FIND ) == 0)
  {
    /*
    screenRestorer.Save();
    const char *msgItems[]=
    {
      g_StartupInfo.GetMsgString(NMessageID::kWaitTitle),
        g_StartupInfo.GetMsgString(NMessageID::kReadingList)
    };
    g_StartupInfo.ShowMessage(0, NULL, msgItems,
      sizeof(msgItems) / sizeof(msgItems[0]), 0);
    */
  }

  UInt32 numItems;
  _folder->GetNumberOfItems(&numItems);
  *panelItems = new PluginPanelItem[numItems];
  try
  {
    for (UInt32 i = 0; i < numItems; i++)
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

void CPlugin::FreeFindData(struct PluginPanelItem *panelItems, int itemsNumber)
{
  for (int i = 0; i < itemsNumber; i++)
    if (panelItems[i].Description != NULL)
      delete []panelItems[i].Description;
  delete []panelItems;
}

void CPlugin::EnterToDirectory(const UString &dirName)
{
  CMyComPtr<IFolderFolder> newFolder;
  UString s = dirName;
  if (dirName == kDotsReplaceStringU)
    s = L"..";
  _folder->BindToFolder(s, &newFolder);
  if (newFolder == NULL)
    if (dirName.IsEmpty())
      return;
    else
      throw 40325;
  _folder = newFolder;
}

int CPlugin::SetDirectory(const char *aszDir, int /* opMode */)
{
  UString path = MultiByteToUnicodeString(aszDir, CP_OEMCP);
  if (path == WSTRING_PATH_SEPARATOR)
  {
    _folder.Release();
    m_ArchiveHandler->BindToRootFolder(&_folder);
  }
  else if (path == L"..")
  {
    CMyComPtr<IFolderFolder> newFolder;
    _folder->BindToParentFolder(&newFolder);
    if (newFolder == NULL)
      throw 40312;
    _folder = newFolder;
  }
  else if (path.IsEmpty())
    EnterToDirectory(path);
  else
  {
    if (path[0] == WCHAR_PATH_SEPARATOR)
    {
      _folder.Release();
      m_ArchiveHandler->BindToRootFolder(&_folder);
      path = path.Mid(1);
    }
    UStringVector pathParts;
    SplitPathToParts(path, pathParts);
    for (int i = 0; i < pathParts.Size(); i++)
      EnterToDirectory(pathParts[i]);
  }
  GetCurrentDir();
  return TRUE;
}

void CPlugin::GetPathParts(UStringVector &pathParts)
{
  pathParts.Clear();
  CMyComPtr<IFolderFolder> folderItem = _folder;
  for (;;)
  {
    CMyComPtr<IFolderFolder> newFolder;
    folderItem->BindToParentFolder(&newFolder);
    if (newFolder == NULL)
      break;
    NCOM::CPropVariant prop;
    if (folderItem->GetFolderProperty(kpidName, &prop) == S_OK)
      if (prop.vt == VT_BSTR)
        pathParts.Insert(0, (const wchar_t *)prop.bstrVal);
    folderItem = newFolder;
  }
}

void CPlugin::GetCurrentDir()
{
  m_CurrentDir.Empty();
  UStringVector pathParts;
  GetPathParts(pathParts);
  for (int i = 0; i < pathParts.Size(); i++)
  {
    m_CurrentDir += WCHAR_PATH_SEPARATOR;
    m_CurrentDir += pathParts[i];
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
  { kpidPath, NMessageID::kPath },
  { kpidName, NMessageID::kName },
  { kpidExtension, NMessageID::kExtension },
  { kpidIsDir, NMessageID::kIsFolder },
  { kpidSize, NMessageID::kSize },
  { kpidPackSize, NMessageID::kPackSize },
  { kpidAttrib, NMessageID::kAttributes },
  { kpidCTime, NMessageID::kCTime },
  { kpidATime, NMessageID::kATime },
  { kpidMTime, NMessageID::kMTime },
  { kpidSolid, NMessageID::kSolid },
  { kpidCommented, NMessageID::kCommented },
  { kpidEncrypted, NMessageID::kEncrypted },
  { kpidSplitBefore, NMessageID::kSplitBefore },
  { kpidSplitAfter, NMessageID::kSplitAfter },
  { kpidDictionarySize, NMessageID::kDictionarySize },
  { kpidCRC, NMessageID::kCRC },
  { kpidType, NMessageID::kType },
  { kpidIsAnti, NMessageID::kAnti },
  { kpidMethod, NMessageID::kMethod },
  { kpidHostOS, NMessageID::kHostOS },
  { kpidFileSystem, NMessageID::kFileSystem },
  { kpidUser, NMessageID::kUser },
  { kpidGroup, NMessageID::kGroup },
  { kpidBlock, NMessageID::kBlock },
  { kpidComment, NMessageID::kComment },
  { kpidPosition, NMessageID::kPosition },
  { kpidNumSubDirs, NMessageID::kNumSubFolders },
  { kpidNumSubFiles, NMessageID::kNumSubFiles },
  { kpidUnpackVer, NMessageID::kUnpackVer },
  { kpidVolume, NMessageID::kVolume },
  { kpidIsVolume, NMessageID::kIsVolume },
  { kpidOffset, NMessageID::kOffset },
  { kpidLinks, NMessageID::kLinks },
  { kpidNumBlocks, NMessageID::kNumBlocks },
  { kpidNumVolumes, NMessageID::kNumVolumes },
  
  { kpidBit64, NMessageID::kBit64 },
  { kpidBigEndian, NMessageID::kBigEndian },
  { kpidCpu, NMessageID::kCpu },
  { kpidPhySize, NMessageID::kPhySize },
  { kpidHeadersSize, NMessageID::kHeadersSize },
  { kpidChecksum, NMessageID::kChecksum },
  { kpidCharacts, NMessageID::kCharacts },
  { kpidVa, NMessageID::kVa },
  { kpidId, NMessageID::kId },
  { kpidShortName, NMessageID::kShortName},
  { kpidCreatorApp, NMessageID::kCreatorApp },
  { kpidSectorSize, NMessageID::kSectorSize },
  { kpidPosixAttrib, NMessageID::kPosixAttrib },
  { kpidLink, NMessageID::kLink },
  { kpidError, NMessageID::kError },
  
  { kpidTotalSize, NMessageID::kTotalSize },
  { kpidFreeSpace, NMessageID::kFreeSpace },
  { kpidClusterSize, NMessageID::kClusterSize },
  { kpidVolumeName, NMessageID::kLabel }
};

static const int kNumPROPIDToName = sizeof(kPROPIDToName) /  sizeof(kPROPIDToName[0]);

static int FindPropertyToName(PROPID propID)
{
  for (int i = 0; i < kNumPROPIDToName; i++)
    if (kPROPIDToName[i].PropID == propID)
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
  { kpidAttrib, "A", 0},
  { kpidCTime, "DC", 14},
  { kpidATime, "DA", 14},
  { kpidMTime, "DM", 14},
  
  { kpidSolid, NULL, 0, 'S'},
  { kpidEncrypted, NULL, 0, 'P'}

  { kpidDictionarySize, IDS_PROPERTY_DICTIONARY_SIZE },
  { kpidSplitBefore, NULL, 'B'},
  { kpidSplitAfter, NULL, 'A'},
  { kpidComment, , NULL, 'C'},
  { kpidCRC, IDS_PROPERTY_CRC }
  // { kpidType, L"Type" }
};

static const int kNumPropertyIDInfos = sizeof(kPropertyIDInfos) /
    sizeof(kPropertyIDInfos[0]);

static int FindPropertyInfo(PROPID propID)
{
  for (int i = 0; i < kNumPropertyIDInfos; i++)
    if (kPropertyIDInfos[i].PropID == propID)
      return i;
  return -1;
}
*/

// char *g_Titles[] = { "a", "f", "v" };
/*
static void SmartAddToString(AString &destString, const char *srcString)
{
  if (!destString.IsEmpty())
    destString += ',';
  destString += srcString;
}
*/

/*
void CPlugin::AddColumn(PROPID propID)
{
  int index = FindPropertyInfo(propID);
  if (index >= 0)
  {
    for (int i = 0; i < m_ProxyHandler->m_InternalProperties.Size(); i++)
    {
      const CArchiveItemProperty &aHandlerProperty = m_ProxyHandler->m_InternalProperties[i];
      if (aHandlerProperty.ID == propID)
        break;
    }
    if (i == m_ProxyHandler->m_InternalProperties.Size())
      return;

    const CPropertyIDInfo &propertyIDInfo = kPropertyIDInfos[index];
    SmartAddToString(PanelModeColumnTypes, propertyIDInfo.FarID);
    char tmp[32];
    itoa(propertyIDInfo.Width, tmp, 10);
    SmartAddToString(PanelModeColumnWidths, tmp);
    return;
  }
}
*/

static AString GetNameOfProp(PROPID propID, const wchar_t *name)
{
  int index = FindPropertyToName(propID);
  if (index < 0)
  {
    if (name)
      return UnicodeStringToMultiByte((const wchar_t *)name, CP_OEMCP);
    char s[32];
    ConvertUInt64ToString(propID, s);
    return s;
  }
  return g_StartupInfo.GetMsgString(kPROPIDToName[index].PluginID);
}

static AString GetNameOfProp2(PROPID propID, const wchar_t *name)
{
  AString s = GetNameOfProp(propID, name);
  if (s.Length() > (kInfoPanelLineSize - 1))
    s = s.Left(kInfoPanelLineSize - 1);
  return s;
}

static AString ConvertSizeToString(UInt64 value)
{
  char s[32];
  ConvertUInt64ToString(value, s);
  int i = MyStringLen(s);
  int pos = sizeof(s) / sizeof(s[0]);
  s[--pos] = L'\0';
  while (i > 3)
  {
    s[--pos] = s[--i];
    s[--pos] = s[--i];
    s[--pos] = s[--i];
    s[--pos] = ' ';
  }
  while (i > 0)
    s[--pos] = s[--i];
  return s + pos;
}

static AString PropToString(const NCOM::CPropVariant &prop, PROPID propID)
{
  AString s;

  if (prop.vt == VT_BSTR)
    s = UnicodeStringToMultiByte(prop.bstrVal, CP_OEMCP);
  else if (prop.vt == VT_BOOL)
  {
    int messageID = VARIANT_BOOLToBool(prop.boolVal) ?
      NMessageID::kYes : NMessageID::kNo;
    return g_StartupInfo.GetMsgString(messageID);
  }
  else if (prop.vt != VT_EMPTY)
  {
    if ((
        propID == kpidSize ||
        propID == kpidPackSize ||
        propID == kpidNumSubDirs ||
        propID == kpidNumSubFiles ||
        propID == kpidNumBlocks ||
        propID == kpidPhySize ||
        propID == kpidHeadersSize ||
        propID == kpidClusterSize
        ) && (prop.vt == VT_UI8 || prop.vt == VT_UI4))
      s = ConvertSizeToString(ConvertPropVariantToUInt64(prop));
    else
      s = UnicodeStringToMultiByte(ConvertPropertyToString(prop, propID), CP_OEMCP);
  }
  s.Replace((char)0xA, ' ');
  s.Replace((char)0xD, ' ');
  return s;
}

static AString PropToString2(const NCOM::CPropVariant &prop, PROPID propID)
{
  AString s = PropToString(prop, propID);
  if (s.Length() > (kInfoPanelLineSize - 1))
    s = s.Left(kInfoPanelLineSize - 1);
  return s;
}

static void AddPropertyString(InfoPanelLine *lines, int &numItems, PROPID propID, const wchar_t *name,
    const NCOM::CPropVariant &prop)
{
  if (prop.vt != VT_EMPTY)
  {
    AString val = PropToString2(prop, propID);
    if (!val.IsEmpty())
    {
      InfoPanelLine &item = lines[numItems++];
      COPY_STR_LIMITED(item.Text, GetNameOfProp2(propID, name));
      COPY_STR_LIMITED(item.Data, val);
    }
  }
}

static void InsertSeparator(InfoPanelLine *lines, int &numItems)
{
  if (numItems < kNumInfoLinesMax)
  {
    InfoPanelLine &item = lines[numItems++];
    MyStringCopy(item.Text, "");
    MyStringCopy(item.Data, "");
    item.Separator = TRUE;
  }
}

void CPlugin::GetOpenPluginInfo(struct OpenPluginInfo *info)
{
  info->StructSize = sizeof(*info);
  info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS| OPIF_USEHIGHLIGHTING|
              OPIF_ADDDOTS | OPIF_COMPAREFATTIME;

  COPY_STR_LIMITED(m_FileNameBuffer, UnicodeStringToMultiByte(m_FileName, CP_OEMCP));
  info->HostFile = m_FileNameBuffer; // test it it is not static
  
  COPY_STR_LIMITED(m_CurrentDirBuffer, UnicodeStringToMultiByte(m_CurrentDir, CP_OEMCP));
  info->CurDir = m_CurrentDirBuffer;

  info->Format = kPluginFormatName;

  UString name;
  {
    UString fullName;
    int index;
    NFile::NDirectory::MyGetFullPathName(m_FileName, fullName, index);
    name = fullName.Mid(index);
  }

  m_PannelTitle =
      UString(L' ') +
      _archiveTypeName +
      UString(L':') +
      name +
      UString(L' ');
  if (!m_CurrentDir.IsEmpty())
  {
    // m_PannelTitle += '\\';
    m_PannelTitle += m_CurrentDir;
  }
 
  COPY_STR_LIMITED(m_PannelTitleBuffer, UnicodeStringToMultiByte(m_PannelTitle, CP_OEMCP));
  info->PanelTitle = m_PannelTitleBuffer;

  memset(m_InfoLines, 0, sizeof(m_InfoLines));
  MyStringCopy(m_InfoLines[0].Text, "");
  m_InfoLines[0].Separator = TRUE;

  MyStringCopy(m_InfoLines[1].Text, g_StartupInfo.GetMsgString(NMessageID::kArchiveType));
  MyStringCopy(m_InfoLines[1].Data, (const char *)UnicodeStringToMultiByte(_archiveTypeName, CP_OEMCP));

  int numItems = 2;

  {
    CMyComPtr<IFolderProperties> folderProperties;
    _folder.QueryInterface(IID_IFolderProperties, &folderProperties);
    if (folderProperties)
    {
      UInt32 numProps;
      if (folderProperties->GetNumberOfFolderProperties(&numProps) == S_OK)
      {
        for (UInt32 i = 0; i < numProps && numItems < kNumInfoLinesMax; i++)
        {
          CMyComBSTR name;
          PROPID propID;
          VARTYPE vt;
          if (folderProperties->GetFolderPropertyInfo(i, &name, &propID, &vt) != S_OK)
            continue;
          NCOM::CPropVariant prop;
          if (_folder->GetFolderProperty(propID, &prop) != S_OK || prop.vt == VT_EMPTY)
            continue;
          
          InfoPanelLine &item = m_InfoLines[numItems++];
          COPY_STR_LIMITED(item.Text, GetNameOfProp2(propID, name));
          COPY_STR_LIMITED(item.Data, PropToString2(prop, propID));
        }
      }
    }
  }

  /*
  if (numItems < kNumInfoLinesMax)
  {
    InsertSeparator(m_InfoLines, numItems);
  }
  */

  {
    CMyComPtr<IGetFolderArcProps> getFolderArcProps;
    _folder.QueryInterface(IID_IGetFolderArcProps, &getFolderArcProps);
    if (getFolderArcProps)
    {
      CMyComPtr<IFolderArcProps> getProps;
      getFolderArcProps->GetFolderArcProps(&getProps);
      if (getProps)
      {
        UInt32 numLevels;
        if (getProps->GetArcNumLevels(&numLevels) != S_OK)
          numLevels = 0;
        for (UInt32 level2 = 0; level2 < numLevels; level2++)
        {
          {
            UInt32 level = numLevels - 1 - level2;
            UInt32 numProps;
            if (getProps->GetArcNumProps(level, &numProps) == S_OK)
            {
              InsertSeparator(m_InfoLines, numItems);
              for (Int32 i = -3; i < (Int32)numProps && numItems < kNumInfoLinesMax; i++)
              {
                CMyComBSTR name;
                PROPID propID;
                VARTYPE vt;
                switch (i)
                {
                  case -3: propID = kpidPath; break;
                  case -2: propID = kpidType; break;
                  case -1: propID = kpidError; break;
                  default:
                    if (getProps->GetArcPropInfo(level, i, &name, &propID, &vt) != S_OK)
                      continue;
                }
                NCOM::CPropVariant prop;
                if (getProps->GetArcProp(level, propID, &prop) != S_OK)
                  continue;
                AddPropertyString(m_InfoLines, numItems, propID, name, prop);
              }
            }
          }
          if (level2 != numLevels - 1)
          {
            UInt32 level = numLevels - 1 - level2;
            UInt32 numProps;
            if (getProps->GetArcNumProps2(level, &numProps) == S_OK)
            {
              InsertSeparator(m_InfoLines, numItems);
              for (Int32 i = 0; i < (Int32)numProps && numItems < kNumInfoLinesMax; i++)
              {
                CMyComBSTR name;
                PROPID propID;
                VARTYPE vt;
                if (getProps->GetArcPropInfo2(level, i, &name, &propID, &vt) != S_OK)
                  continue;
                NCOM::CPropVariant prop;
                if (getProps->GetArcProp2(level, propID, &prop) != S_OK)
                  continue;
                AddPropertyString(m_InfoLines, numItems, propID, name, prop);
              }
            }
          }
        }
      }
    }
  }

  //m_InfoLines[1].Separator = 0;

  info->InfoLines = m_InfoLines;
  info->InfoLinesNumber = numItems;

  
  info->DescrFiles = NULL;
  info->DescrFilesNumber = 0;

  PanelModeColumnTypes.Empty();
  PanelModeColumnWidths.Empty();

  /*
  AddColumn(kpidName);
  AddColumn(kpidSize);
  AddColumn(kpidPackedSize);
  AddColumn(kpidMTime);
  AddColumn(kpidCTime);
  AddColumn(kpidATime);
  AddColumn(kpidAttrib);
  
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
        NFile::NFind::NAttributes::IsDir(pluginPanelItem.FindData.dwFileAttributes))
    return S_FALSE;
  int itemIndex = (int)pluginPanelItem.UserData;

  CObjectVector<CArchiveItemProperty> properties;
  UInt32 numProps;
  RINOK(_folder->GetNumberOfProperties(&numProps));
  int i;
  for (i = 0; i < (int)numProps; i++)
  {
    CMyComBSTR name;
    PROPID propID;
    VARTYPE vt;
    RINOK(_folder->GetPropertyInfo(i, &name, &propID, &vt));
    CArchiveItemProperty prop;
    prop.Type = vt;
    prop.ID = propID;
    if (prop.ID  == kpidPath)
      prop.ID  = kpidName;
    prop.Name = GetNameOfProp(propID, name);
    properties.Add(prop);
  }

  int size = 2;
  CRecordVector<CInitDialogItem> initDialogItems;
  
  int xSize = 70;
  CInitDialogItem idi =
  { DI_DOUBLEBOX, 3, 1, xSize - 4, size - 2, false, false, 0, false, NMessageID::kProperties, NULL, NULL };
  initDialogItems.Add(idi);
  AStringVector values;

  for (i = 0; i < properties.Size(); i++)
  {
    const CArchiveItemProperty &property = properties[i];

    CInitDialogItem idi =
      { DI_TEXT, 5, 3 + i, 0, 0, false, false, 0, false, 0, NULL, NULL };
    int index = FindPropertyToName(property.ID);
    if (index < 0)
    {
      idi.DataMessageId = -1;
      idi.DataString = property.Name;
    }
    else
      idi.DataMessageId = kPROPIDToName[index].PluginID;
    initDialogItems.Add(idi);
    
    NCOM::CPropVariant prop;
    RINOK(_folder->GetProperty(itemIndex, property.ID, &prop));
    AString s = PropToString(prop, property.ID);
    values.Add(s);
    
    {
      CInitDialogItem idi =
      { DI_TEXT, 30, 3 + i, 0, 0, false, false, 0, false, -1, NULL, NULL };
      initDialogItems.Add(idi);
    }
  }

  int numLines = values.Size();
  for (i = 0; i < numLines; i++)
  {
    CInitDialogItem &idi = initDialogItems[1 + i * 2 + 1];
    idi.DataString = values[i];
  }
  
  int numDialogItems = initDialogItems.Size();
  
  CRecordVector<FarDialogItem> dialogItems;
  dialogItems.Reserve(numDialogItems);
  for (i = 0; i < numDialogItems; i++)
    dialogItems.Add(FarDialogItem());
  g_StartupInfo.InitDialogItems(&initDialogItems.Front(),
      &dialogItems.Front(), numDialogItems);
  
  int maxLen = 0;
  for (i = 0; i < numLines; i++)
  {
    FarDialogItem &dialogItem = dialogItems[1 + i * 2];
    int len = (int)strlen(dialogItem.Data);
    if (len > maxLen)
      maxLen = len;
  }
  int maxLen2 = 0;
  const int kSpace = 10;
  for (i = 0; i < numLines; i++)
  {
    FarDialogItem &dialogItem = dialogItems[1 + i * 2 + 1];
    int len = (int)strlen(dialogItem.Data);
    if (len > maxLen2)
      maxLen2 = len;
    dialogItem.X1 = maxLen + kSpace;
  }
  size = numLines + 6;
  xSize = maxLen + kSpace + maxLen2 + 5;
  FarDialogItem &firstDialogItem = dialogItems.Front();
  firstDialogItem.Y2 = size - 2;
  firstDialogItem.X2 = xSize - 4;
  
  /* int askCode = */ g_StartupInfo.ShowDialog(xSize, size, NULL, &dialogItems.Front(), numDialogItems);
  return S_OK;
}

int CPlugin::ProcessKey(int key, unsigned int controlState)
{
  if (controlState == PKF_CONTROL && key == 'A')
  {
    HRESULT result = ShowAttributesWindow();
    if (result == S_OK)
      return TRUE;
    if (result == S_FALSE)
      return FALSE;
    throw "Error";
  }
  if ((controlState & PKF_ALT) != 0 && key == VK_F6)
  {
    UString folderPath;
    if (!NFile::NDirectory::GetOnlyDirPrefix(m_FileName, folderPath))
      return FALSE;
    PanelInfo panelInfo;
    g_StartupInfo.ControlGetActivePanelInfo(panelInfo);
    GetFilesReal(panelInfo.SelectedItems,
        panelInfo.SelectedItemsNumber, FALSE,
        UnicodeStringToMultiByte(folderPath, CP_OEMCP), OPM_SILENT, true);
    g_StartupInfo.Control(this, FCTL_UPDATEPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_REDRAWPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_UPDATEANOTHERPANEL, NULL);
    g_StartupInfo.Control(this, FCTL_REDRAWANOTHERPANEL, NULL);
    return TRUE;
  }

  return FALSE;
}
