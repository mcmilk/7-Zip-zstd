// ZipRegistry.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/Registry.h"
#include "Windows/Synchronization.h"

#include "ZipRegistry.h"

using namespace NWindows;
using namespace NRegistry;

static NSynchronization::CCriticalSection g_CS;
#define CS_LOCK NSynchronization::CCriticalSectionLock lock(g_CS);

static const TCHAR *kCuPrefix = TEXT("Software") TEXT(STRING_PATH_SEPARATOR) TEXT("7-Zip") TEXT(STRING_PATH_SEPARATOR);

static CSysString GetKeyPath(const CSysString &path) { return kCuPrefix + path; }

static LONG OpenMainKey(CKey &key, LPCTSTR keyName)
{
  return key.Open(HKEY_CURRENT_USER, GetKeyPath(keyName), KEY_READ);
}

static LONG CreateMainKey(CKey &key, LPCTSTR keyName)
{
  return key.Create(HKEY_CURRENT_USER, GetKeyPath(keyName));
}

namespace NExtract
{

static const TCHAR *kKeyName = TEXT("Extraction");

static const TCHAR *kExtractMode = TEXT("ExtractMode");
static const TCHAR *kOverwriteMode = TEXT("OverwriteMode");
static const TCHAR *kShowPassword = TEXT("ShowPassword");
static const TCHAR *kPathHistory = TEXT("PathHistory");

void CInfo::Save() const
{
  CS_LOCK
  CKey key;
  CreateMainKey(key, kKeyName);
  key.SetValue(kExtractMode, (UInt32)PathMode);
  key.SetValue(kOverwriteMode, (UInt32)OverwriteMode);
  key.SetValue(kShowPassword, ShowPassword);
  key.RecurseDeleteKey(kPathHistory);
  key.SetValue_Strings(kPathHistory, Paths);
}


void CInfo::Load()
{
  PathMode = NPathMode::kCurrentPathnames;
  OverwriteMode = NOverwriteMode::kAskBefore;
  ShowPassword = false;
  Paths.Clear();

  CS_LOCK
  CKey key;
  if (OpenMainKey(key, kKeyName) != ERROR_SUCCESS)
    return;
  
  key.GetValue_Strings(kPathHistory, Paths);
  UInt32 v;
  if (key.QueryValue(kExtractMode, v) == ERROR_SUCCESS && v <= NPathMode::kNoPathnames)
    PathMode = (NPathMode::EEnum)v;
  if (key.QueryValue(kOverwriteMode, v) == ERROR_SUCCESS && v <= NOverwriteMode::kAutoRenameExisting)
    OverwriteMode = (NOverwriteMode::EEnum)v;
  key.GetValue_IfOk(kShowPassword, ShowPassword);
}

}

namespace NCompression
{

static const TCHAR *kKeyName = TEXT("Compression");

static const TCHAR *kArcHistory = TEXT("ArcHistory");
static const WCHAR *kArchiver = L"Archiver";
static const TCHAR *kShowPassword = TEXT("ShowPassword");
static const TCHAR *kEncryptHeaders = TEXT("EncryptHeaders");

static const TCHAR *kOptionsKeyName = TEXT("Options");

static const TCHAR *kLevel = TEXT("Level");
static const TCHAR *kDictionary = TEXT("Dictionary");
static const TCHAR *kOrder = TEXT("Order");
static const TCHAR *kBlockSize = TEXT("BlockSize");
static const TCHAR *kNumThreads = TEXT("NumThreads");
static const WCHAR *kMethod = L"Method";
static const WCHAR *kOptions = L"Options";
static const WCHAR *kEncryptionMethod = L"EncryptionMethod";

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
    value = (UInt32)-1;
}

void CInfo::Save() const
{
  CS_LOCK

  CKey key;
  CreateMainKey(key, kKeyName);
  key.SetValue(kLevel, (UInt32)Level);
  key.SetValue(kArchiver, ArcType);
  key.SetValue(kShowPassword, ShowPassword);
  key.SetValue(kEncryptHeaders, EncryptHeaders);
  key.RecurseDeleteKey(kArcHistory);
  key.SetValue_Strings(kArcHistory, ArcPaths);

  key.RecurseDeleteKey(kOptionsKeyName);
  {
    CKey optionsKey;
    optionsKey.Create(key, kOptionsKeyName);
    for (int i = 0; i < Formats.Size(); i++)
    {
      const CFormatOptions &fo = Formats[i];
      CKey fk;
      fk.Create(optionsKey, fo.FormatID);
      
      SetRegUInt32(fk, kLevel, fo.Level);
      SetRegUInt32(fk, kDictionary, fo.Dictionary);
      SetRegUInt32(fk, kOrder, fo.Order);
      SetRegUInt32(fk, kBlockSize, fo.BlockLogSize);
      SetRegUInt32(fk, kNumThreads, fo.NumThreads);

      SetRegString(fk, kMethod, fo.Method);
      SetRegString(fk, kOptions, fo.Options);
      SetRegString(fk, kEncryptionMethod, fo.EncryptionMethod);
    }
  }
}

void CInfo::Load()
{
  ArcPaths.Clear();
  Formats.Clear();

  Level = 5;
  ArcType = L"7z";
  ShowPassword = false;
  EncryptHeaders = false;

  CS_LOCK
  CKey key;

  if (OpenMainKey(key, kKeyName) != ERROR_SUCCESS)
    return;

  key.GetValue_Strings(kArcHistory, ArcPaths);
  
  {
    CKey optionsKey;
    if (optionsKey.Open(key, kOptionsKeyName, KEY_READ) == ERROR_SUCCESS)
    {
      CSysStringVector formatIDs;
      optionsKey.EnumKeys(formatIDs);
      for (int i = 0; i < formatIDs.Size(); i++)
      {
        CKey fk;
        CFormatOptions fo;
        fo.FormatID = formatIDs[i];
        if (fk.Open(optionsKey, fo.FormatID, KEY_READ) == ERROR_SUCCESS)
        {
          GetRegString(fk, kOptions, fo.Options);
          GetRegString(fk, kMethod, fo.Method);
          GetRegString(fk, kEncryptionMethod, fo.EncryptionMethod);

          GetRegUInt32(fk, kLevel, fo.Level);
          GetRegUInt32(fk, kDictionary, fo.Dictionary);
          GetRegUInt32(fk, kOrder, fo.Order);
          GetRegUInt32(fk, kBlockSize, fo.BlockLogSize);
          GetRegUInt32(fk, kNumThreads, fo.NumThreads);

          Formats.Add(fo);
        }
      }
    }
  }

  UString a;
  if (key.QueryValue(kArchiver, a) == ERROR_SUCCESS)
    ArcType = a;
  key.GetValue_IfOk(kLevel, Level);
  key.GetValue_IfOk(kShowPassword, ShowPassword);
  key.GetValue_IfOk(kEncryptHeaders, EncryptHeaders);
}

}

static const TCHAR *kOptionsInfoKeyName = TEXT("Options");

namespace NWorkDir
{
static const TCHAR *kWorkDirType = TEXT("WorkDirType");
static const WCHAR *kWorkDirPath = L"WorkDirPath";
static const TCHAR *kTempRemovableOnly = TEXT("TempRemovableOnly");


void CInfo::Save()const
{
  CS_LOCK
  CKey key;
  CreateMainKey(key, kOptionsInfoKeyName);
  key.SetValue(kWorkDirType, (UInt32)Mode);
  key.SetValue(kWorkDirPath, Path);
  key.SetValue(kTempRemovableOnly, ForRemovableOnly);
}

void CInfo::Load()
{
  SetDefault();

  CS_LOCK
  CKey key;
  if (OpenMainKey(key, kOptionsInfoKeyName) != ERROR_SUCCESS)
    return;

  UInt32 dirType;
  if (key.QueryValue(kWorkDirType, dirType) != ERROR_SUCCESS)
    return;
  switch (dirType)
  {
    case NMode::kSystem:
    case NMode::kCurrent:
    case NMode::kSpecified:
      Mode = (NMode::EEnum)dirType;
  }
  if (key.QueryValue(kWorkDirPath, Path) != ERROR_SUCCESS)
  {
    Path.Empty();
    if (Mode == NMode::kSpecified)
      Mode = NMode::kSystem;
  }
  key.GetValue_IfOk(kTempRemovableOnly, ForRemovableOnly);
}

}

static const TCHAR *kCascadedMenu = TEXT("CascadedMenu");
static const TCHAR *kContextMenu = TEXT("ContextMenu");

void CContextMenuInfo::Save() const
{
  CS_LOCK
  CKey key;
  CreateMainKey(key, kOptionsInfoKeyName);
  key.SetValue(kCascadedMenu, Cascaded);
  key.SetValue(kContextMenu, Flags);
}

void CContextMenuInfo::Load()
{
  Cascaded = true;
  Flags = (UInt32)-1;
  CS_LOCK
  CKey key;
  if (OpenMainKey(key, kOptionsInfoKeyName) != ERROR_SUCCESS)
    return;
  key.GetValue_IfOk(kCascadedMenu, Cascaded);
  key.GetValue_IfOk(kContextMenu, Flags);
}
