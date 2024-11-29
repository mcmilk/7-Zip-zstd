// OverwriteDialogFar.h

#ifndef ZIP7_INC_OVERWRITE_DIALOG_FAR_H
#define ZIP7_INC_OVERWRITE_DIALOG_FAR_H

#include "../../../Common/MyString.h"
#include "../../../Common/MyTypes.h"

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
