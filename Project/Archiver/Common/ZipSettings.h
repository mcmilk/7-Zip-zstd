// ZipSettings.h

#pragma once

#ifndef __ZIPSETTINGS_H
#define __ZIPSETTINGS_H

#include "Common/String.h"
namespace NZipSettings {

namespace NExtraction{
  
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

namespace NCompression{
  
  struct CFormatOptions
  {
    CSysString FormatID;
    CSysString Options;
  };

  struct CInfo
  {
    CSysStringVector HistoryArchives;
    bool MethodDefined;
    BYTE Method;
    bool LastClassIDDefined;
    CLSID LastClassID;


    bool SolidMode;
    CObjectVector<CFormatOptions> FormatOptionsVector;

    bool ShowPassword;

    void SetMethod(BYTE method) {Method = method; MethodDefined = true; }
    void SetLastClassID(const CLSID &lastClassID) 
      { LastClassID = lastClassID; LastClassIDDefined = true; }
    // bool Maximize;
  };

  /*
  struct CDefinedStatus
  {
    bool Method;
    bool LastClassID;
    // bool Maximize;
  };
  */
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
    CSysString Path;
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


}

#endif
