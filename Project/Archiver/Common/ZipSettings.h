// ZipSettings.h

#pragma once

#ifndef __ZIPSETTINGS_H
#define __ZIPSETTINGS_H

#include "Common/String.h"
namespace NZipSettings {

struct CColumnInfo
{
  PROPID PropID;
  bool IsVisible;
  // UINT32 Order;
  UINT32 Width;
};

typedef std::vector<CColumnInfo> CColumnInfoVector;

struct CListViewInfo
{
  CColumnInfoVector ColumnInfoVector;
  int SortIndex;
  bool Ascending;

  int FindColumnWithID(PROPID anID) const;
  void OrderItems();
};


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

    void SetMethod(BYTE aMethod) {Method = aMethod; MethodDefined = true; }
    void SetLastClassID(const CLSID &aLastClassID) 
      { LastClassID = aLastClassID; LastClassIDDefined = true; }
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
