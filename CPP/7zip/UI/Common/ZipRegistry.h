// ZipRegistry.h

#ifndef __ZIPREGISTRY_H
#define __ZIPREGISTRY_H

#include "Common/MyString.h"
#include "Common/Types.h"
#include "ExtractMode.h"

namespace NExtract
{
  struct CInfo
  {
    NPathMode::EEnum PathMode;
    NOverwriteMode::EEnum OverwriteMode;
    UStringVector Paths;
    bool ShowPassword;
  };
}

namespace NCompression {
  
  struct CFormatOptions
  {
    CSysString FormatID;
    UString Options;
    UString Method;
    UString EncryptionMethod;
    UInt32 Level;
    UInt32 Dictionary;
    UInt32 Order;
    UInt32 BlockLogSize;
    UInt32 NumThreads;
    void ResetForLevelChange() 
    { 
      BlockLogSize = NumThreads = Level = Dictionary = Order = UInt32(-1); 
      Method.Empty();
      // EncryptionMethod.Empty();
      // Options.Empty();
    }
    CFormatOptions() { ResetForLevelChange(); }
  };

  struct CInfo
  {
    UStringVector HistoryArchives;
    UInt32 Level;
    UString ArchiveType;

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

void SaveExtractionInfo(const NExtract::CInfo &info);
void ReadExtractionInfo(NExtract::CInfo &info);

void SaveCompressionInfo(const NCompression::CInfo &info);
void ReadCompressionInfo(NCompression::CInfo &info);

void SaveWorkDirInfo(const NWorkDir::CInfo &info);
void ReadWorkDirInfo(NWorkDir::CInfo &info);

void SaveCascadedMenu(bool enabled);
bool ReadCascadedMenu();

void SaveContextMenuStatus(UInt32 value);
bool ReadContextMenuStatus(UInt32 &value);

#endif
