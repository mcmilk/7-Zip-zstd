// ZipRegistry.cpp

#include "StdAfx.h"

#include "ZipRegistry.h"
#include "Windows/COM.h"
#include "Windows/Synchronization.h"

#include "Windows/FileDir.h"

using namespace NZipSettings;

using namespace NWindows;
using namespace NCOM;
using namespace NRegistry;

static const TCHAR *kCUBasePath = _T("Software\\7-ZIP");

static const TCHAR *kArchiversKeyName = _T("Archivers");


////////////////////////////////////////////////////////////
// CZipRegistryManager

CZipRegistryManager::CZipRegistryManager()
{
  m_CUKey.Create(HKEY_CURRENT_USER, kCUBasePath);
}

static const TCHAR *kCulumnsValueName = _T("Columns");
// static const TCHAR *kCulumnSortValueName = _T("ColumnSort");

#pragma pack( push, PragmaColumnInfoSpec)
#pragma pack( push, 1)

class CColumnInfoSpec
{
  UINT32 PropID;
  BYTE IsVisible;
  // UINT32 Order;
  UINT32 Width;
public:
  void GetFromColumnInfo(const CColumnInfo &aSrc) 
  {
    PropID = aSrc.PropID;
    IsVisible = aSrc.IsVisible ? 1: 0;
    // Order = aSrc.Order;
    Width = aSrc.Width;
  }
  void PutColumnInfo(CColumnInfo &aDest) 
  {
    aDest.PropID = PropID;
    aDest.IsVisible = (IsVisible != 0);
    // aDest.Order = Order;
    aDest.Width = Width;
  }
};

struct CColumnHeader
{
  UINT32 Version;
  UINT32 SortIndex;
  BYTE Ascending;
};


#pragma pack(pop)
#pragma pack(pop, PragmaColumnInfoSpec)

static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;

static const UINT32 kAscendingMask = (1UL << 31);

static const UINT32 kColumnInfoVersion = 0;

void CZipRegistryManager::SaveListViewInfo(const CLSID &aClassID, const CListViewInfo &aViewInfo)
{
  const CColumnInfoVector &aColumnInfoList = aViewInfo.ColumnInfoVector;
  CByteDynamicBuffer aBuffer;
  UINT32 aDataSize = sizeof(CColumnHeader) + sizeof(CColumnInfoSpec) * aColumnInfoList.size();
  aBuffer.EnsureCapacity(aDataSize);

  BYTE *aDataPointer = (BYTE *)aBuffer;

  CColumnHeader &aColumnHeader = *(CColumnHeader *)aDataPointer;
  aColumnHeader.Version = kColumnInfoVersion;
  aColumnHeader.SortIndex = aViewInfo.SortIndex;
  aColumnHeader.Ascending = aViewInfo.Ascending? 1: 0;

  CColumnInfoSpec *aDestItems = (CColumnInfoSpec *)(aDataPointer + sizeof(CColumnHeader));
  for(int i = 0; i < aColumnInfoList.size(); i++)
  {
    CColumnInfoSpec &aColumnInfoSpec = aDestItems[i];
    aColumnInfoSpec.GetFromColumnInfo(aColumnInfoList[i]);
  }
  {
    NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
    CSysString aKeyName = kArchiversKeyName;
    aKeyName += kKeyNameDelimiter;
    aKeyName += GUIDToString(aClassID);
    CKey aKey;
    aKey.Create(m_CUKey, aKeyName);
    aKey.SetValue(kCulumnsValueName, aDataPointer, aDataSize);
  }
}

void CZipRegistryManager::ReadListViewInfo(const CLSID &aClassID, CListViewInfo &aViewInfo)
{
  CColumnInfoVector &aColumnInfoList = aViewInfo.ColumnInfoVector;
  aViewInfo.SortIndex = -1;
  aViewInfo.Ascending = true;
  aColumnInfoList.clear();
  CByteDynamicBuffer aBuffer;
  UINT32 aSize;
  {
    NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
    CSysString aKeyName = kArchiversKeyName;
    aKeyName += kKeyNameDelimiter;
    aKeyName += GUIDToString(aClassID);
    CKey aKey;
    if(aKey.Open(m_CUKey, aKeyName, KEY_READ) != ERROR_SUCCESS)
      return;
    if (aKey.QueryValue(kCulumnsValueName, aBuffer, aSize) != ERROR_SUCCESS)
      return;
  }
  if (aSize < sizeof(CColumnHeader))
    return;
  BYTE *aDataPointer = (BYTE *)aBuffer;
  const CColumnHeader &aColumnHeader = *(CColumnHeader*)aDataPointer;
  if (aColumnHeader.Version != kColumnInfoVersion)
    return;
  aViewInfo.Ascending = (aColumnHeader.Ascending != 0);
  aViewInfo.SortIndex = aColumnHeader.SortIndex;

  aSize -= sizeof(CColumnHeader);
  if (aSize % sizeof(CColumnHeader) != 0)
    return;
  int aNumItems = aSize / sizeof(CColumnInfoSpec);
  CColumnInfoSpec *aSpecItems = (CColumnInfoSpec *)(aDataPointer + sizeof(CColumnHeader));;
  for(int i = 0; i < aNumItems; i++)
  {
    CColumnInfo aColumnInfo;
    aSpecItems[i].PutColumnInfo(aColumnInfo);
    aColumnInfoList.push_back(aColumnInfo);
  }
}


//////////////////////
// ExtractionInfo

static const TCHAR *kExtractionInfoKeyName = _T("Extraction");

static const TCHAR *kExtractionPathHistoryKeyName = _T("PathHistory");
static const TCHAR *kExtractionExtractModeValueName = _T("ExtarctMode");
static const TCHAR *kExtractionOverwriteModeValueName = _T("OverwriteMode");

void CZipRegistryManager::SaveExtractionInfo(const NExtraction::CInfo &anInfo)
{
  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CKey anExtractionKey;
  anExtractionKey.Create(m_CUKey, kExtractionInfoKeyName);
  anExtractionKey.RecurseDeleteKey(kExtractionPathHistoryKeyName);
  {
    CKey aPathHistoryKey;
    aPathHistoryKey.Create(anExtractionKey, kExtractionPathHistoryKeyName);
    for(int i = 0; i < anInfo.Paths.Size(); i++)
    {
      TCHAR aNumberString[16];
      _ltot(i, aNumberString, 10);
      aPathHistoryKey.SetValue(aNumberString, anInfo.Paths[i]);
    }
  }
  anExtractionKey.SetValue(kExtractionExtractModeValueName, UINT32(anInfo.PathMode));
  anExtractionKey.SetValue(kExtractionOverwriteModeValueName, UINT32(anInfo.OverwriteMode));
}

void CZipRegistryManager::ReadExtractionInfo(NExtraction::CInfo &anInfo)
{
  anInfo.Paths.Clear();
  anInfo.PathMode = NExtraction::NPathMode::kFullPathnames;
  anInfo.OverwriteMode = NExtraction::NOverwriteMode::kAskBefore;

  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CKey anExtractionKey;
  if(anExtractionKey.Open(m_CUKey, kExtractionInfoKeyName, KEY_READ) != ERROR_SUCCESS)
    return;
  
  {
    CKey aPathHistoryKey;
    if(aPathHistoryKey.Open(anExtractionKey, kExtractionPathHistoryKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    {        
      while(true)
      {
        TCHAR aNumberString[16];
        _ltot(anInfo.Paths.Size(), aNumberString, 10);
        CSysString aPath;
        if (aPathHistoryKey.QueryValue(aNumberString, aPath) != ERROR_SUCCESS)
          break;
        anInfo.Paths.Add(aPath);
      }
    }
  }
  UINT32 anExtractModeIndex;
  anExtractionKey.QueryValue(kExtractionExtractModeValueName, anExtractModeIndex);
  switch (anExtractModeIndex)
  {
    case NExtraction::NPathMode::kFullPathnames:
    case NExtraction::NPathMode::kCurrentPathnames:
    case NExtraction::NPathMode::kNoPathnames:
      anInfo.PathMode = NExtraction::NPathMode::EEnum(anExtractModeIndex);
      break;
  }
  UINT32 anOverwriteModeIndex;
  anExtractionKey.QueryValue(kExtractionOverwriteModeValueName, anOverwriteModeIndex);
  switch (anOverwriteModeIndex)
  {
    case NExtraction::NOverwriteMode::kAskBefore:
    case NExtraction::NOverwriteMode::kWithoutPrompt:
    case NExtraction::NOverwriteMode::kSkipExisting:
    case NExtraction::NOverwriteMode::kAutoRename:
      anInfo.OverwriteMode = NExtraction::NOverwriteMode::EEnum(anOverwriteModeIndex);
      break;
  }
}

///////////////////////////////////
// CompressionInfo

static const TCHAR *kCompressionInfoKeyName = _T("Compression");

static const TCHAR *kCompressionHistoryArchivesKeyName = _T("ArcHistory");
static const TCHAR *kCompressionMethodValueName = _T("Method");
static const TCHAR *kCompressionLastClassIDValueName = _T("Archiver");
// static const TCHAR *kCompressionMaximizeValueName = _T("Maximize");

static const TCHAR *kCompressionOptionsKeyName = _T("Options");
static const TCHAR *kCompressionOptionsSolidValueName = _T("Solid");
static const TCHAR *kCompressionOptionsOptionsValueName = _T("Options");

void CZipRegistryManager::SaveCompressionInfo(const NCompression::CInfo &anInfo)
{
  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);

  CKey aCompressionKey;
  aCompressionKey.Create(m_CUKey, kCompressionInfoKeyName);
  aCompressionKey.RecurseDeleteKey(kCompressionHistoryArchivesKeyName);
  {
    CKey aHistoryArchivesKey;
    aHistoryArchivesKey.Create(aCompressionKey, kCompressionHistoryArchivesKeyName);
    for(int i = 0; i < anInfo.HistoryArchives.Size(); i++)
    {
      TCHAR aNumberString[16];
      _ltot(i, aNumberString, 10);
      aHistoryArchivesKey.SetValue(aNumberString, anInfo.HistoryArchives[i]);
    }
  }

  aCompressionKey.RecurseDeleteKey(kCompressionOptionsKeyName);
  {
    CKey anOptionsKey;
    anOptionsKey.Create(aCompressionKey, kCompressionOptionsKeyName);
    anOptionsKey.SetValue(kCompressionOptionsSolidValueName, anInfo.SolidMode);
    for(int i = 0; i < anInfo.FormatOptionsVector.Size(); i++)
    {
      const NCompression::CFormatOptions &aFormatOptions = anInfo.FormatOptionsVector[i];
      CKey aFormatKey;
      aFormatKey.Create(anOptionsKey, aFormatOptions.FormatID);
      aFormatKey.SetValue(kCompressionOptionsOptionsValueName, aFormatOptions.Options);
    }
  }

  if (anInfo.MethodDefined)
    aCompressionKey.SetValue(kCompressionMethodValueName, UINT32(anInfo.Method));
  if (anInfo.LastClassIDDefined)
    aCompressionKey.SetValue(kCompressionLastClassIDValueName, GUIDToString(anInfo.LastClassID));

  // aCompressionKey.SetValue(kCompressionMaximizeValueName, anInfo.Maximize);
}

void CZipRegistryManager::ReadCompressionInfo(NCompression::CInfo &anInfo)
{
  anInfo.HistoryArchives.Clear();

  anInfo.SolidMode = false;
  anInfo.FormatOptionsVector.Clear();

  anInfo.MethodDefined = false;
  anInfo.LastClassIDDefined = false;
  // aDefinedStatus.Maximize = false;


  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CKey aCompressionKey;
  if(aCompressionKey.Open(m_CUKey, kCompressionInfoKeyName, KEY_READ) != ERROR_SUCCESS)
    return;
  
  {
    CKey aHistoryArchivesKey;
    if(aHistoryArchivesKey.Open(aCompressionKey, kCompressionHistoryArchivesKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    {        
      while(true)
      {
        TCHAR aNumberString[16];
        _ltot(anInfo.HistoryArchives.Size(), aNumberString, 10);
        CSysString aPath;
        if (aHistoryArchivesKey.QueryValue(aNumberString, aPath) != ERROR_SUCCESS)
          break;
        anInfo.HistoryArchives.Add(aPath);
      }
    }
  }

  
  {
    CKey anOptionsKey;
    if(anOptionsKey.Open(aCompressionKey, kCompressionOptionsKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    { 
      bool aSolid = false;
      if (anOptionsKey.QueryValue(kCompressionOptionsSolidValueName, aSolid) == ERROR_SUCCESS)
        anInfo.SolidMode = aSolid;
      CSysStringVector aFormatIDs;
      anOptionsKey.EnumKeys(aFormatIDs);
      for(int i = 0; i < aFormatIDs.Size(); i++)
      {
        NCompression::CFormatOptions aFormatOptions;
        aFormatOptions.FormatID = aFormatIDs[i];
        CKey aFormatKey;
        if(aFormatKey.Open(anOptionsKey, aFormatOptions.FormatID, KEY_READ) == ERROR_SUCCESS)
           if (aFormatKey.QueryValue(kCompressionOptionsOptionsValueName, 
                aFormatOptions.Options) == ERROR_SUCCESS)
              anInfo.FormatOptionsVector.Add(aFormatOptions);

      }
    }
  }

  UINT32 aMethod;
  if (aCompressionKey.QueryValue(kCompressionMethodValueName, aMethod) == ERROR_SUCCESS)
  { 
    anInfo.Method = BYTE(aMethod);
    anInfo.MethodDefined = true;
  }
  CSysString aClassIDString;
  if (aCompressionKey.QueryValue(kCompressionLastClassIDValueName, aClassIDString) == ERROR_SUCCESS)
  { 
    anInfo.LastClassIDDefined = 
        (StringToGUID(aClassIDString, anInfo.LastClassID) == NOERROR);
  }
  /*
  if (aCompressionKey.QueryValue(kCompressionMethodValueName, anInfo.Maximize) == ERROR_SUCCESS)
    aDefinedStatus.Maximize = true;
  */
}




///////////////////////////////////
// WorkDirInfo

static const TCHAR *kOptionsInfoKeyName = _T("Options");

static const TCHAR *kWorkDirTypeValueName = _T("WorkDirType");
static const TCHAR *kWorkDirPathValueName = _T("WorkDirPath");
static const TCHAR *kTempRemovableOnlyValueName = _T("TempRemovableOnly");

void CZipRegistryManager::SaveWorkDirInfo(const NWorkDir::CInfo &anInfo)
{
  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CKey anOptionsKey;
  anOptionsKey.Create(m_CUKey, kOptionsInfoKeyName);
  anOptionsKey.SetValue(kWorkDirTypeValueName, UINT32(anInfo.Mode));
  anOptionsKey.SetValue(kWorkDirPathValueName, anInfo.Path);
  anOptionsKey.SetValue(kTempRemovableOnlyValueName, anInfo.ForRemovableOnly);
}

void CZipRegistryManager::ReadWorkDirInfo(NWorkDir::CInfo &anInfo)
{
  anInfo.SetDefault();

  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CKey anOptionsKey;
  if(anOptionsKey.Open(m_CUKey, kOptionsInfoKeyName, KEY_READ) != ERROR_SUCCESS)
    return;

  UINT32 aType;
  if (anOptionsKey.QueryValue(kWorkDirTypeValueName, aType) != ERROR_SUCCESS)
    return;
  switch (aType)
  {
    case NWorkDir::NMode::kSystem:
    case NWorkDir::NMode::kCurrent:
    case NWorkDir::NMode::kSpecified:
      anInfo.Mode = NWorkDir::NMode::EEnum(aType);
  }
  if (anOptionsKey.QueryValue(kWorkDirPathValueName, anInfo.Path) != ERROR_SUCCESS)
  {
    anInfo.Path.Empty();
    if (anInfo.Mode == NWorkDir::NMode::kSpecified)
      anInfo.Mode = NWorkDir::NMode::kSystem;
  }
  if (anOptionsKey.QueryValue(kTempRemovableOnlyValueName, anInfo.ForRemovableOnly) != ERROR_SUCCESS)
    anInfo.SetForRemovableOnlyDefault();
}
