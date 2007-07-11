// OverwriteDialog.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "OverwriteDialog.h"

#include "Common/StringConvert.h"

#include "Windows/FileName.h"
#include "Windows/Defs.h"
#include "Windows/PropVariantConversions.h"

#include "FarUtils.h"
#include "Messages.h"

using namespace NWindows;
using namespace NFar;

namespace NOverwriteDialog {

struct CFileInfoStrings
{
  CSysString Size;
  CSysString Time;
};

void SetFileInfoStrings(const CFileInfo &fileInfo,
    CFileInfoStrings &fileInfoStrings) 
{
  char buffer[256];

  if (fileInfo.SizeIsDefined)
  {
    sprintf(buffer, "%I64u ", fileInfo.Size);
    fileInfoStrings.Size = buffer;
    fileInfoStrings.Size += g_StartupInfo.GetMsgString(NMessageID::kOverwriteBytes);
  }
  else
  {
    fileInfoStrings.Size = "";
  }

  FILETIME localFileTime;
  fileInfoStrings.Time.Empty();
  if (fileInfo.TimeIsDefined)
  {
    if (!FileTimeToLocalFileTime(&fileInfo.Time, &localFileTime))
      throw 4190402;
    UString timeString = ConvertFileTimeToString(localFileTime);
    fileInfoStrings.Time = g_StartupInfo.GetMsgString(NMessageID::kOverwriteModifiedOn);
    fileInfoStrings.Time += " ";
    fileInfoStrings.Time += GetSystemString(timeString, CP_OEMCP);
  }
}

NResult::EEnum Execute(const CFileInfo &oldFileInfo, const CFileInfo &newFileInfo)
{
  const int kYSize = 20;
  const int kXSize = 76;
  
  CFileInfoStrings oldFileInfoStrings;
  CFileInfoStrings newFileInfoStrings;

  SetFileInfoStrings(oldFileInfo, oldFileInfoStrings);
  SetFileInfoStrings(newFileInfo, newFileInfoStrings);

  struct CInitDialogItem anInitItems[]={
    { DI_DOUBLEBOX, 3, 1, kXSize - 4, kYSize - 2, false, false, 0, false, NMessageID::kOverwriteTitle, NULL, NULL },
    { DI_TEXT, 5, 2, 0, 0, false, false, 0, false, NMessageID::kOverwriteMessage1, NULL, NULL },
    
    { DI_TEXT, 3, 3, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, "", NULL  },  
    
    { DI_TEXT, 5, 4, 0, 0, false, false, 0, false, NMessageID::kOverwriteMessageWouldYouLike, NULL, NULL },

    { DI_TEXT, 7, 6, 0, 0, false, false, 0, false,  -1, oldFileInfo.Name, NULL },
    { DI_TEXT, 7, 7, 0, 0, false, false, 0, false,  -1, oldFileInfoStrings.Size, NULL },
    { DI_TEXT, 7, 8, 0, 0, false, false, 0, false,  -1, oldFileInfoStrings.Time, NULL },

    { DI_TEXT, 5, 10, 0, 0, false, false, 0, false, NMessageID::kOverwriteMessageWithtTisOne, NULL, NULL },
    
    { DI_TEXT, 7, 12, 0, 0, false, false, 0, false,  -1, newFileInfo.Name, NULL },
    { DI_TEXT, 7, 13, 0, 0, false, false, 0, false,  -1, newFileInfoStrings.Size, NULL },
    { DI_TEXT, 7, 14, 0, 0, false, false, 0, false,  -1, newFileInfoStrings.Time, NULL },

    { DI_TEXT, 3, kYSize - 5, 0, 0, false, false, DIF_BOXCOLOR|DIF_SEPARATOR, false, -1, "", NULL  },  
        
    { DI_BUTTON, 0, kYSize - 4, 0, 0, true, false, DIF_CENTERGROUP, true, NMessageID::kOverwriteYes, NULL, NULL  },
    { DI_BUTTON, 0, kYSize - 4, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kOverwriteYesToAll, NULL, NULL  },
    { DI_BUTTON, 0, kYSize - 4, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kOverwriteNo, NULL, NULL  },
    { DI_BUTTON, 0, kYSize - 4, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kOverwriteNoToAll, NULL, NULL  },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kOverwriteAutoRename, NULL, NULL  },
    { DI_BUTTON, 0, kYSize - 3, 0, 0, false, false, DIF_CENTERGROUP, false, NMessageID::kOverwriteCancel, NULL, NULL  }
  };
  
  const int kNumDialogItems = sizeof(anInitItems) / sizeof(anInitItems[0]);
  FarDialogItem aDialogItems[kNumDialogItems];
  g_StartupInfo.InitDialogItems(anInitItems, aDialogItems, kNumDialogItems);
  int anAskCode = g_StartupInfo.ShowDialog(kXSize, kYSize, 
      NULL, aDialogItems, kNumDialogItems);
  const int kButtonStartPos = kNumDialogItems - 6;
  if (anAskCode >= kButtonStartPos && anAskCode < kNumDialogItems)
    return NResult::EEnum(anAskCode - kButtonStartPos);
  return NResult::kCancel;
}

}

