// OverwriteDialog.h

#ifndef OVERWRITEDIALOG_H
#define OVERWRITEDIALOG_H

#include "Common/MyString.h"
#include "Common/Types.h"

namespace NOverwriteDialog {

struct CFileInfo
{
  bool SizeIsDefined;
  bool TimeIsDefined;
  UInt64 Size;
  FILETIME Time;
  UString Name;
};

namespace NResult
{
  enum EEnum
  {
    kYes,
    kYesToAll,
    kNo,
    kNoToAll,
    kAutoRename,
    kCancel
  };
}

NResult::EEnum Execute(const CFileInfo &oldFileInfo, const CFileInfo &newFileInfo);

}

#endif
