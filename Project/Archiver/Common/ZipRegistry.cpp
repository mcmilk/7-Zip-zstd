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

namespace NZipRegistryManager
{
  
static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;

//////////////////////
// ExtractionInfo

static const TCHAR *kExtractionInfoKeyName = _T("Extraction");

static const TCHAR *kExtractionPathHistoryKeyName = _T("PathHistory");
static const TCHAR *kExtractionExtractModeValueName = _T("ExtarctMode");
static const TCHAR *kExtractionOverwriteModeValueName = _T("OverwriteMode");

static CSysString GetKeyPath(const CSysString &path)
{
  return CSysString(kCUBasePath) + CSysString('\\') + CSysString(path);
}

void SaveExtractionInfo(const NExtraction::CInfo &info)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey extractionKey;
  extractionKey.Create(HKEY_CURRENT_USER, GetKeyPath(kExtractionInfoKeyName));
  extractionKey.RecurseDeleteKey(kExtractionPathHistoryKeyName);
  {
    CKey pathHistoryKey;
    pathHistoryKey.Create(extractionKey, kExtractionPathHistoryKeyName);
    for(int i = 0; i < info.Paths.Size(); i++)
    {
      TCHAR numberString[16];
      _ltot(i, numberString, 10);
      pathHistoryKey.SetValue(numberString, info.Paths[i]);
    }
  }
  extractionKey.SetValue(kExtractionExtractModeValueName, UINT32(info.PathMode));
  extractionKey.SetValue(kExtractionOverwriteModeValueName, UINT32(info.OverwriteMode));
}

void ReadExtractionInfo(NExtraction::CInfo &info)
{
  info.Paths.Clear();
  info.PathMode = NExtraction::NPathMode::kFullPathnames;
  info.OverwriteMode = NExtraction::NOverwriteMode::kAskBefore;

  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey extractionKey;
  if(extractionKey.Open(HKEY_CURRENT_USER, GetKeyPath(kExtractionInfoKeyName), KEY_READ) != ERROR_SUCCESS)
    return;
  
  {
    CKey pathHistoryKey;
    if(pathHistoryKey.Open(extractionKey, kExtractionPathHistoryKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    {        
      while(true)
      {
        TCHAR numberString[16];
        _ltot(info.Paths.Size(), numberString, 10);
        CSysString path;
        if (pathHistoryKey.QueryValue(numberString, path) != ERROR_SUCCESS)
          break;
        info.Paths.Add(path);
      }
    }
  }
  UINT32 extractModeIndex;
  extractionKey.QueryValue(kExtractionExtractModeValueName, extractModeIndex);
  switch (extractModeIndex)
  {
    case NExtraction::NPathMode::kFullPathnames:
    case NExtraction::NPathMode::kCurrentPathnames:
    case NExtraction::NPathMode::kNoPathnames:
      info.PathMode = NExtraction::NPathMode::EEnum(extractModeIndex);
      break;
  }
  UINT32 overwriteModeIndex;
  extractionKey.QueryValue(kExtractionOverwriteModeValueName, overwriteModeIndex);
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

void SaveCompressionInfo(const NCompression::CInfo &info)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);

  CKey compressionKey;
  compressionKey.Create(HKEY_CURRENT_USER, GetKeyPath(kCompressionInfoKeyName));
  compressionKey.RecurseDeleteKey(kCompressionHistoryArchivesKeyName);
  {
    CKey historyArchivesKey;
    historyArchivesKey.Create(compressionKey, kCompressionHistoryArchivesKeyName);
    for(int i = 0; i < info.HistoryArchives.Size(); i++)
    {
      TCHAR numberString[16];
      _ltot(i, numberString, 10);
      historyArchivesKey.SetValue(numberString, info.HistoryArchives[i]);
    }
  }

  compressionKey.RecurseDeleteKey(kCompressionOptionsKeyName);
  {
    CKey optionsKey;
    optionsKey.Create(compressionKey, kCompressionOptionsKeyName);
    optionsKey.SetValue(kCompressionOptionsSolidValueName, info.SolidMode);
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
    compressionKey.SetValue(kCompressionLastClassIDValueName, GUIDToString(info.LastClassID));

  // compressionKey.SetValue(kCompressionMaximizeValueName, info.Maximize);
}

void ReadCompressionInfo(NCompression::CInfo &info)
{
  info.HistoryArchives.Clear();

  info.SolidMode = false;
  info.FormatOptionsVector.Clear();

  info.MethodDefined = false;
  info.LastClassIDDefined = false;
  // definedStatus.Maximize = false;


  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey compressionKey;

  if(compressionKey.Open(HKEY_CURRENT_USER, 
      GetKeyPath(kCompressionInfoKeyName), KEY_READ) != ERROR_SUCCESS)
    return;
  
  {
    CKey historyArchivesKey;
    if(historyArchivesKey.Open(compressionKey, kCompressionHistoryArchivesKeyName, KEY_READ) == 
        ERROR_SUCCESS)
    {        
      while(true)
      {
        TCHAR numberString[16];
        _ltot(info.HistoryArchives.Size(), numberString, 10);
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
      if (optionsKey.QueryValue(kCompressionOptionsSolidValueName, solid) == ERROR_SUCCESS)
        info.SolidMode = solid;
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
    info.LastClassIDDefined = 
        (StringToGUID(classIDString, info.LastClassID) == NOERROR);
  }
  /*
  if (compressionKey.QueryValue(kCompressionMethodValueName, info.Maximize) == ERROR_SUCCESS)
    definedStatus.Maximize = true;
  */
}


///////////////////////////////////
// WorkDirInfo

static const TCHAR *kOptionsInfoKeyName = _T("Options");

static const TCHAR *kWorkDirTypeValueName = _T("WorkDirType");
static const TCHAR *kWorkDirPathValueName = _T("WorkDirPath");
static const TCHAR *kTempRemovableOnlyValueName = _T("TempRemovableOnly");

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
}