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

// static const TCHAR *kArchiversKeyName = TEXT("Archivers");

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

void SaveExtractionInfo(const NExtraction::CInfo &info)
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
      TCHAR numberString[16];
      ConvertUINT64ToString(i, numberString);
      pathHistoryKey.SetValue(numberString, info.Paths[i]);
    }
  }
  extractionKey.SetValue(kExtractionExtractModeValueName, UINT32(info.PathMode));
  extractionKey.SetValue(kExtractionOverwriteModeValueName, UINT32(info.OverwriteMode));
  extractionKey.SetValue(kExtractionShowPasswordValueName, info.ShowPassword);
}

void ReadExtractionInfo(NExtraction::CInfo &info)
{
  info.Paths.Clear();
  info.PathMode = NExtraction::NPathMode::kFullPathnames;
  info.OverwriteMode = NExtraction::NOverwriteMode::kAskBefore;
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
      while(true)
      {
        TCHAR numberString[16];
        ConvertUINT64ToString(info.Paths.Size(), numberString);
        CSysString path;
        if (pathHistoryKey.QueryValue(numberString, path) != ERROR_SUCCESS)
          break;
        info.Paths.Add(path);
      }
    }
  }
  UINT32 extractModeIndex;
  if (extractionKey.QueryValue(kExtractionExtractModeValueName, extractModeIndex) == ERROR_SUCCESS)
  {
    switch (extractModeIndex)
    {
      case NExtraction::NPathMode::kFullPathnames:
      case NExtraction::NPathMode::kCurrentPathnames:
      case NExtraction::NPathMode::kNoPathnames:
        info.PathMode = NExtraction::NPathMode::EEnum(extractModeIndex);
        break;
    }
  }
  UINT32 overwriteModeIndex;
  if (extractionKey.QueryValue(kExtractionOverwriteModeValueName, overwriteModeIndex) == ERROR_SUCCESS)
  {
    switch (overwriteModeIndex)
    {
      case NExtraction::NOverwriteMode::kAskBefore:
      case NExtraction::NOverwriteMode::kWithoutPrompt:
      case NExtraction::NOverwriteMode::kSkipExisting:
      case NExtraction::NOverwriteMode::kAutoRename:
        info.OverwriteMode = NExtraction::NOverwriteMode::EEnum(overwriteModeIndex);
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
static const TCHAR *kCompressionMethodValueName = TEXT("Method");
static const TCHAR *kCompressionLastClassIDValueName = TEXT("Archiver");
static const TCHAR *kCompressionShowPasswordValueName = TEXT("ShowPassword");
static const TCHAR *kCompressionEncryptHeadersValueName = TEXT("EncryptHeaders");
// static const TCHAR *kCompressionMaximizeValueName = TEXT("Maximize");

static const TCHAR *kCompressionOptionsKeyName = TEXT("Options");
static const TCHAR *kSolid = TEXT("Solid");
static const TCHAR *kMultiThread = TEXT("Multithread");
static const TCHAR *kCompressionOptionsOptionsValueName = TEXT("Options");

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
      TCHAR numberString[16];
      ConvertUINT64ToString(i, numberString);
      historyArchivesKey.SetValue(numberString, info.HistoryArchives[i]);
    }
  }

  compressionKey.RecurseDeleteKey(kCompressionOptionsKeyName);
  {
    CKey optionsKey;
    optionsKey.Create(compressionKey, kCompressionOptionsKeyName);
    optionsKey.SetValue(kSolid, info.Solid);
    optionsKey.SetValue(kMultiThread, info.MultiThread);
    for(int i = 0; i < info.FormatOptionsVector.Size(); i++)
    {
      const NCompression::CFormatOptions &formatOptions = info.FormatOptionsVector[i];
      CKey formatKey;
      formatKey.Create(optionsKey, formatOptions.FormatID);
      formatKey.SetValue(kCompressionOptionsOptionsValueName, formatOptions.Options);
    }
  }

  if (info.MethodDefined)
    compressionKey.SetValue(kCompressionMethodValueName, UINT32(info.Method));
  if (info.LastClassIDDefined)
    compressionKey.SetValue(kCompressionLastClassIDValueName, 
        // GUIDToString(info.LastClassID)
        GetSystemString(info.LastArchiveType)
    );

  compressionKey.SetValue(kCompressionShowPasswordValueName, info.ShowPassword);
  compressionKey.SetValue(kCompressionEncryptHeadersValueName, info.EncryptHeaders);
  // compressionKey.SetValue(kCompressionMaximizeValueName, info.Maximize);
}

void ReadCompressionInfo(NCompression::CInfo &info)
{
  info.HistoryArchives.Clear();

  info.Solid = true;
  info.MultiThread = false;
  info.FormatOptionsVector.Clear();

  info.MethodDefined = false;
  info.LastClassIDDefined = false;
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
      while(true)
      {
        TCHAR numberString[16];
        ConvertUINT64ToString(info.HistoryArchives.Size(), numberString);
        CSysString path;
        if (historyArchivesKey.QueryValue(numberString, path) != ERROR_SUCCESS)
          break;
        info.HistoryArchives.Add(path);
      }
    }
  }

  
  {
    CKey optionsKey;
    if(optionsKey.Open(compressionKey, kCompressionOptionsKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    { 
      bool solid = false;
      if (optionsKey.QueryValue(kSolid, solid) == ERROR_SUCCESS)
        info.Solid = solid;
      bool multiThread = false;
      if (optionsKey.QueryValue(kMultiThread, multiThread) == ERROR_SUCCESS)
        info.MultiThread = multiThread;
      CSysStringVector formatIDs;
      optionsKey.EnumKeys(formatIDs);
      for(int i = 0; i < formatIDs.Size(); i++)
      {
        NCompression::CFormatOptions formatOptions;
        formatOptions.FormatID = formatIDs[i];
        CKey formatKey;
        if(formatKey.Open(optionsKey, formatOptions.FormatID, KEY_READ) == ERROR_SUCCESS)
           if (formatKey.QueryValue(kCompressionOptionsOptionsValueName, 
                formatOptions.Options) == ERROR_SUCCESS)
              info.FormatOptionsVector.Add(formatOptions);

      }
    }
  }

  UINT32 method;
  if (compressionKey.QueryValue(kCompressionMethodValueName, method) == ERROR_SUCCESS)
  { 
    info.Method = BYTE(method);
    info.MethodDefined = true;
  }
  CSysString classIDString;
  if (compressionKey.QueryValue(kCompressionLastClassIDValueName, classIDString) == ERROR_SUCCESS)
  { 
    info.LastClassIDDefined = true;
    info.LastArchiveType = GetUnicodeString(classIDString);

    /*
    info.LastClassIDDefined = 
        (StringToGUID(classIDString, info.LastClassID) == NOERROR);
    */
  }
  if (compressionKey.QueryValue(kCompressionShowPasswordValueName, 
      info.ShowPassword) != ERROR_SUCCESS)
    info.ShowPassword = false;
  if (compressionKey.QueryValue(kCompressionEncryptHeadersValueName, 
      info.EncryptHeaders) != ERROR_SUCCESS)
    info.EncryptHeaders = false;
  /*
  if (compressionKey.QueryValue(kCompressionMethodValueName, info.Maximize) == ERROR_SUCCESS)
    definedStatus.Maximize = true;
  */
}


///////////////////////////////////
// WorkDirInfo

static const TCHAR *kOptionsInfoKeyName = TEXT("Options");

static const TCHAR *kWorkDirTypeValueName = TEXT("WorkDirType");
static const TCHAR *kWorkDirPathValueName = TEXT("WorkDirPath");
static const TCHAR *kTempRemovableOnlyValueName = TEXT("TempRemovableOnly");
static const TCHAR *kCascadedMenuValueName = TEXT("CascadedMenu");
static const TCHAR *kContextMenuValueName = TEXT("ContextMenu");

void SaveWorkDirInfo(const NWorkDir::CInfo &info)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  optionsKey.Create(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName));
  optionsKey.SetValue(kWorkDirTypeValueName, UINT32(info.Mode));
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

  UINT32 dirType;
  if (optionsKey.QueryValue(kWorkDirTypeValueName, dirType) != ERROR_SUCCESS)
    return;
  switch (dirType)
  {
    case NWorkDir::NMode::kSystem:
    case NWorkDir::NMode::kCurrent:
    case NWorkDir::NMode::kSpecified:
      info.Mode = NWorkDir::NMode::EEnum(dirType);
  }
  if (optionsKey.QueryValue(kWorkDirPathValueName, info.Path) != ERROR_SUCCESS)
  {
    info.Path.Empty();
    if (info.Mode == NWorkDir::NMode::kSpecified)
      info.Mode = NWorkDir::NMode::kSystem;
  }
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
  { return ReadOption(kCascadedMenuValueName, false); }


static void SaveValue(const TCHAR *value, UINT32 valueToWrite)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  optionsKey.Create(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName));
  optionsKey.SetValue(value, valueToWrite);
}

static bool ReadValue(const TCHAR *value, UINT32 &result)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey optionsKey;
  if(optionsKey.Open(HKEY_CURRENT_USER, GetKeyPath(kOptionsInfoKeyName), KEY_READ) != ERROR_SUCCESS)
    return false;
  return (optionsKey.QueryValue(value, result) == ERROR_SUCCESS);
}

void SaveContextMenuStatus(UINT32 value)
  { SaveValue(kContextMenuValueName, value); }

bool ReadContextMenuStatus(UINT32 &value)
  { return  ReadValue(kContextMenuValueName, value); }
