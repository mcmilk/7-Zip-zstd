// ZipRegistry.h

#pragma once

#ifndef __ZIPREGISTRY_H
#define __ZIPREGISTRY_H

#include "Common/String.h"

namespace NExtraction {
  
  namespace NPathMode
  {
    enum EEnum
    {
      kFullPathnames,
      kCurrentPathnames,
      kNoPathnames
    };
  }
  
  namespace NOverwriteMode
  {
    enum EEnum
    {
      kAskBefore,
      kWithoutPrompt,
      kSkipExisting,
      kAutoRename,
      kAutoRenameExisting
    };
  }
  
  struct CInfo
  {
    NPathMode::EEnum PathMode;
    NOverwriteMode::EEnum OverwriteMode;
    CSysStringVector Paths;
    bool ShowPassword;
  };
}

namespace NCompression {
  
  struct CFormatOptions
  {
    CSysString FormatID;
    CSysString Options;
  };

  struct CInfo
  {
    CSysStringVector HistoryArchives;
    // bool LevelIsDefined;
    UINT32 Level;
    UString ArchiveType;

    bool Solid;
    bool MultiThread;
    CObjectVector<CFormatOptions> FormatOptionsVector;

    bool ShowPassword;
    bool EncryptHeaders;
  };
}

namespace NWorkDir{
  
  namespace NMode
  {
    enum EEnum
    {
      kSystem,
      kCurrent,
      kSpecified
    };
  }
  struct CInfo
  {
    NMode::EEnum Mode;
    UString Path;
    bool ForRemovableOnly;
    void SetForRemovableOnlyDefault() { ForRemovableOnly = true; }
    void SetDefault()
    {
      Mode = NMode::kSystem;
      Path.Empty();
      SetForRemovableOnlyDefault();
    }
  };
}

void SaveExtractionInfo(const NExtraction::CInfo &info);
void ReadExtractionInfo(NExtraction::CInfo &info);

void SaveCompressionInfo(const NCompression::CInfo &info);
void ReadCompressionInfo(NCompression::CInfo &info);

void SaveWorkDirInfo(const NWorkDir::CInfo &info);
void ReadWorkDirInfo(NWorkDir::CInfo &info);

void SaveCascadedMenu(bool enabled);
bool ReadCascadedMenu();

void SaveContextMenuStatus(UINT32 value);
bool ReadContextMenuStatus(UINT32 &value);

#endif