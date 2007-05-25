// ZipRegistry.cpp

#include "StdAfx.h"

#include "ZipRegistry.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/Synchronization.h"
#include "Windows/Registry.h"

#include "Windows/FileDir.h"

using namespace NWindows;
using namespace NRegistry;

static const TCHAR *kCUBasePath = TEXT("Software\\7-ZIP");

static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;

//////////////////////
// ExtractionInfo

static const TCHAR *kExtractionKeyName = TEXT("Extraction");

static const TCHAR *kExtractionPathHistoryKeyName = TEXT("PathHistory");
static const TCHAR *kExtractionExtractModeValueName = TEXT("ExtarctMode");
static const TCHAR *kExtractionOverwriteModeValueName = TEXT("OverwriteMode");
static const TCHAR *kExtractionShowPasswordValueName = TEXT("ShowPassword");

static CSysString GetKeyPath(const CSysString &path)
{
  return CSysString(kCUBasePath) + CSysString('\\') + CSysString(path);
}

void SaveExtractionInfo(const NExtract::CInfo &info)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey extractionKey;
  extractionKey.Create(HKEY_CURRENT_USER, GetKeyPath(kExtractionKeyName));
  extractionKey.RecurseDeleteKey(kExtractionPathHistoryKeyName);
  {
    CKey pathHistoryKey;
    pathHistoryKey.Create(extractionKey, kExtractionPathHistoryKeyName);
    for(int i = 0; i < info.Paths.Size(); i++)
    {
      wchar_t numberString[16];
      ConvertUInt64ToString(i, numberString);
      pathHistoryKey.SetValue(numberString, info.Paths[i]);
    }
  }
  extractionKey.SetValue(kExtractionExtractModeValueName, UInt32(info.PathMode));
  extractionKey.SetValue(kExtractionOverwriteModeValueName, UInt32(info.OverwriteMode));
  extractionKey.SetValue(kExtractionShowPasswordValueName, info.ShowPassword);
}

void ReadExtractionInfo(NExtract::CInfo &info)
{
  info.Paths.Clear();
  info.PathMode = NExtract::NPathMode::kCurrentPathnames;
  info.OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
  info.ShowPassword = false;

  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey extractionKey;
  if(extractionKey.Open(HKEY_CURRENT_USER, GetKeyPath(kExtractionKeyName), KEY_READ) != ERROR_SUCCESS)
    return;
  
  {
    CKey pathHistoryKey;
    if(pathHistoryKey.Open(extractionKey, kExtractionPathHistoryKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    {        
      for (;;)
      {
        wchar_t numberString[16];
        ConvertUInt64ToString(info.Paths.Size(), numberString);
        UString path;
        if (pathHistoryKey.QueryValue(numberString, path) != ERROR_SUCCESS)
          break;
        info.Paths.Add(path);
      }
    }
  }
  UInt32 extractModeIndex;
  if (extractionKey.QueryValue(kExtractionExtractModeValueName, extractModeIndex) == ERROR_SUCCESS)
  {
    switch (extractModeIndex)
    {
      case NExtract::NPathMode::kFullPathnames:
      case NExtract::NPathMode::kCurrentPathnames:
      case NExtract::NPathMode::kNoPathnames:
        info.PathMode = NExtract::NPathMode::EEnum(extractModeIndex);
        break;
    }
  }
  UInt32 overwriteModeIndex;
  if (extractionKey.QueryValue(kExtractionOverwriteModeValueName, overwriteModeIndex) == ERROR_SUCCESS)
  {
    switch (overwriteModeIndex)
    {
      case NExtract::NOverwriteMode::kAskBefore:
      case NExtract::NOverwriteMode::kWithoutPrompt:
      case NExtract::NOverwriteMode::kSkipExisting:
      case NExtract::NOverwriteMode::kAutoRename:
      case NExtract::NOverwriteMode::kAutoRenameExisting:
        info.OverwriteMode = NExtract::NOverwriteMode::EEnum(overwriteModeIndex);
        break;
    }
  }
  if (extractionKey.QueryValue(kExtractionShowPasswordValueName, 
      info.ShowPassword) != ERROR_SUCCESS)
    info.ShowPassword = false;
}

///////////////////////////////////
// CompressionInfo

static const TCHAR *kCompressionKeyName = TEXT("Compression");

static const TCHAR *kCompressionHistoryArchivesKeyName = TEXT("ArcHistory");
static const TCHAR *kCompressionLevelValueName = TEXT("Level");
static const TCHAR *kCompressionLastFormatValueName = TEXT("Archiver");
static const TCHAR *kCompressionShowPasswordValueName = TEXT("ShowPassword");
static const TCHAR *kCompressionEncryptHeadersValueName = TEXT("EncryptHeaders");

static const TCHAR *kCompressionOptionsKeyName = TEXT("Options");
// static const TCHAR *kSolid = TEXT("Solid");
// static const TCHAR *kMultiThread = TEXT("Multithread");

static const WCHAR *kCompressionOptions = L"Options";
static const TCHAR *kCompressionLevel = TEXT("Level");
static const WCHAR *kCompressionMethod = L"Method";
static const WCHAR *kEncryptionMethod = L"EncryptionMethod";
static const TCHAR *kCompressionDictionary = TEXT("Dictionary");
static const TCHAR *kCompressionOrder = TEXT("Order");
static const TCHAR *kCompressionNumThreads = TEXT("NumThreads");
static const TCHAR *kCompressionBlockSize = TEXT("BlockSize");


static void SetRegString(CKey &key, const WCHAR *name, const UString &value)
{
  if (value.IsEmpty())
    key.DeleteValue(name);
  else
    key.SetValue(name, value);
}

static void SetRegUInt32(CKey &key, const TCHAR *name, UInt32 value)
{
  if (value == (UInt32)-1)
    key.DeleteValue(name);
  else
    key.SetValue(name, value);
}

static void GetRegString(CKey &key, const WCHAR *name, UString &value)
{
  if (key.QueryValue(name, value) != ERROR_SUCCESS)
    value.Empty();
}

static void GetRegUInt32(CKey &key, const TCHAR *name, UInt32 &value)
{
  if (key.QueryValue(name, value) != ERROR_SUCCESS)
    value = UInt32(-1);
}

void SaveCompressionInfo(const NCompression::CInfo &info)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);

  CKey compressionKey;
  compressionKey.Create(HKEY_CURRENT_USER, GetKeyPath(kCompressionKeyName));
  compressionKey.RecurseDeleteKey(kCompressionHistoryArchivesKeyName);
  {
    CKey historyArchivesKey;
    historyArchivesKey.Create(compressionKey, kCompressionHistoryArchivesKeyName);
    for(int i = 0; i < info.HistoryArchives.Size(); i++)
    {
      wchar_t numberString[16];
      ConvertUInt64ToString(i, numberString);
      historyArchivesKey.SetValue(numberString, info.HistoryArchives[i]);
    }
  }

  // compressionKey.SetValue(kSolid, info.Solid);
  // compressionKey.SetValue(kMultiThread, info.MultiThread);
  compressionKey.RecurseDeleteKey(kCompressionOptionsKeyName);
  {
    CKey optionsKey;
    optionsKey.Create(compressionKey, kCompressionOptionsKeyName);
    for(int i = 0; i < info.FormatOptionsVector.Size(); i++)
    {
      const NCompression::CFormatOptions &fo = info.FormatOptionsVector[i];
      CKey formatKey;
      formatKey.Create(optionsKey, fo.FormatID);
      
      SetRegString(formatKey, kCompressionOptions, fo.Options);
      SetRegString(formatKey, kCompressionMethod, fo.Method);
      SetRegString(formatKey, kEncryptionMethod, fo.EncryptionMethod);

      SetRegUInt32(formatKey, kCompressionLevel, fo.Level);
      SetRegUInt32(formatKey, kCompressionDictionary, fo.Dictionary);
      SetRegUInt32(formatKey, kCompressionOrder, fo.Order);
      SetRegUInt32(formatKey, kCompressionBlockSize, fo.BlockLogSize);
      SetRegUInt32(formatKey, kCompressionNumThreads, fo.NumThreads);
    }
  }

  compressionKey.SetValue(kCompressionLevelValueName, UInt32(info.Level));
  compressionKey.SetValue(kCompressionLastFormatValueName, GetSystemString(info.ArchiveType));

  compressionKey.SetValue(kCompressionShowPasswordValueName, info.ShowPassword);
  compressionKey.SetValue(kCompressionEncryptHeadersValueName, info.EncryptHeaders);
  // compressionKey.SetValue(kCompressionMaximizeValueName, info.Maximize);
}

void ReadCompressionInfo(NCompression::CInfo &info)
{
  info.HistoryArchives.Clear();

  // info.Solid = true;
  // info.MultiThread = IsMultiProcessor();
  info.FormatOptionsVector.Clear();

  info.Level = 5;
  info.ArchiveType = L"7z";
  // definedStatus.Maximize = false;
  info.ShowPassword = false;
  info.EncryptHeaders = false;

  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey compressionKey;

  if(compressionKey.Open(HKEY_CURRENT_USER, 
      GetKeyPath(kCompressionKeyName), KEY_READ) != ERROR_SUCCESS)
    return;
  
  {
    CKey historyArchivesKey;
    if(historyArchivesKey.Open(compressionKey, kCompressionHistoryArchivesKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    {        
      for (;;)
      {
        wchar_t numberString[16];
        ConvertUInt64ToString(info.HistoryArchives.Size(), numberString);
        UString path;
        if (historyArchivesKey.QueryValue(numberString, path) != ERROR_SUCCESS)
          break;
        info.HistoryArchives.Add(path);
      }
    }
  }

  
  /*
  bool solid = false;
  if (compressionKey.QueryValue(kSolid, solid) == ERROR_SUCCESS)
    info.Solid = solid;
  bool multiThread = false;
  if (compressionKey.QueryValue(kMultiThread, multiThread) == ERROR_SUCCESS)
    info.MultiThread = multiThread;
  */

  {
    CKey optionsKey;
    if(optionsKey.Open(compressionKey, kCompressionOptionsKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    { 
      CSysStringVector formatIDs;
      optionsKey.EnumKeys(formatIDs);
      for(int i = 0; i < formatIDs.Size(); i++)
      {
        CKey formatKey;
        NCompression::CFormatOptions fo;
        fo.FormatID = formatIDs[i];
        if(formatKey.Open(optionsKey, fo.FormatID, KEY_READ) == ERROR_SUCCESS)
        {
          GetRegString(formatKey, kCompressionOptions, fo.Options);
          GetRegString(formatKey, kCompressionMethod, fo.Method);
          GetRegString(formatKey, kEncryptionMethod, fo.EncryptionMethod);

          GetRegUInt32(formatKey, kCompressionLevel, fo.Level);
          GetRegUInt32(formatKey, kCompressionDictionary, fo.Dictionary);
          GetRegUInt32(formatKey, kCompressionOrder, fo.Order);
          GetRegUInt32(formatKey, kCompressionBlockSize, fo.BlockLogSize);
          GetRegUInt32(formatKey, kCompressionNumThreads, fo.NumThreads);

          info.FormatOptionsVector.Add(fo);
        }

      }
    }
  }

  UInt32 level;
  if (compressionKey.QueryValue(kCompressionLevelValueName, level) == ERROR_SUCCESS)
    info.Level = level;
  CSysString archiveType;
  if (compressionKey.QueryValue(kCompressionLastFormatValueName, archiveType) == ERROR_SUCCESS)
    info.ArchiveType = GetUnicodeString(archiveType);
  if (compressionKey.QueryValue(kCompressionShowPasswordValueName, 
      info.ShowPassword) != ERROR_SUCCESS)
    info.ShowPassword = false;
  if (compressionKey.QueryValue(kCompressionEncryptHeadersValueName, 
      info.EncryptHeaders) != ERROR_SUCCESS)
    info.EncryptHeaders = false;
  /*
  if (compressionKey.QueryValue(kCompressionLevelValueName, info.Maximize) == ERROR_SUCCESS)
    definedStatus.Maximize = true;
  */
}


///////////////////////////////////
// WorkDirInfo

static const TCHAR *kOptionsInfoKeyName = TEXT("Options");

static const TCHAR *kWorkDirTypeValueName = TEXT("WorkDirType");
static const WCHAR *kWorkDirPathValueName = L"WorkDirPath";
static const TCHAR *kTempRemovableOnlyValueName = TEXT("TempRemovableOnly");
static const TCHAR *kCascadedMenuValueName = TEXT("CascadedMenu");
static const TCHAR *kContextMenuValueName = TEXT("ContextMenu");

void SaveWorkDirInfo(const NWorkDir::CInfo &info)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  optionsKey.Create(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName));
  optionsKey.SetValue(kWorkDirTypeValueName, UInt32(info.Mode));
  optionsKey.SetValue(kWorkDirPathValueName, info.Path);
  optionsKey.SetValue(kTempRemovableOnlyValueName, info.ForRemovableOnly);
}

void ReadWorkDirInfo(NWorkDir::CInfo &info)
{
  info.SetDefault();

  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  if(optionsKey.Open(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName), KEY_READ) != ERROR_SUCCESS)
    return;

  UInt32 dirType;
  if (optionsKey.QueryValue(kWorkDirTypeValueName, dirType) != ERROR_SUCCESS)
    return;
  switch (dirType)
  {
    case NWorkDir::NMode::kSystem:
    case NWorkDir::NMode::kCurrent:
    case NWorkDir::NMode::kSpecified:
      info.Mode = NWorkDir::NMode::EEnum(dirType);
  }
  UString sysWorkDir;
  if (optionsKey.QueryValue(kWorkDirPathValueName, sysWorkDir) != ERROR_SUCCESS)
  {
    info.Path.Empty();
    if (info.Mode == NWorkDir::NMode::kSpecified)
      info.Mode = NWorkDir::NMode::kSystem;
  }
  info.Path = GetUnicodeString(sysWorkDir);
  if (optionsKey.QueryValue(kTempRemovableOnlyValueName, info.ForRemovableOnly) != ERROR_SUCCESS)
    info.SetForRemovableOnlyDefault();
}

static void SaveOption(const TCHAR *value, bool enabled)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  optionsKey.Create(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName));
  optionsKey.SetValue(value, enabled);
}

static bool ReadOption(const TCHAR *value, bool defaultValue)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  if(optionsKey.Open(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName), KEY_READ) != ERROR_SUCCESS)
    return defaultValue;
  bool enabled;
  if (optionsKey.QueryValue(value, enabled) != ERROR_SUCCESS)
    return defaultValue;
  return enabled;
}

void SaveCascadedMenu(bool show)
  { SaveOption(kCascadedMenuValueName, show); }
bool ReadCascadedMenu()
  { return ReadOption(kCascadedMenuValueName, true); }


static void SaveValue(const TCHAR *value, UInt32 valueToWrite)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  optionsKey.Create(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName));
  optionsKey.SetValue(value, valueToWrite);
}

static bool ReadValue(const TCHAR *value, UInt32 &result)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  if(optionsKey.Open(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName), KEY_READ) != ERROR_SUCCESS)
    return false;
  return (optionsKey.QueryValue(value, result) == ERROR_SUCCESS);
}

void SaveContextMenuStatus(UInt32 value)
  { SaveValue(kContextMenuValueName, value); }

bool ReadContextMenuStatus(UInt32 &value)
  { return  ReadValue(kContextMenuValueName, value); }
