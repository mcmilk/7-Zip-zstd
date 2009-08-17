// OverwriteDialog.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/FileName.h"
#include "Windows/Defs.h"
#include "Windows/ResourceString.h"
#include "Windows/Control/Static.h"
#include "Windows/PropVariantConversions.h"

#include "FormatUtils.h"
#include "OverwriteDialog.h"

// #include "../resource.h"

#ifdef LANG
#include "LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG
static CIDLangPair kIDLangPairs[] =
{
  { IDC_STATIC_OVERWRITE_HEADER,         0x02000901},
  { IDC_STATIC_OVERWRITE_QUESTION_BEGIN, 0x02000902 },
  { IDC_STATIC_OVERWRITE_QUESTION_END,   0x02000903 },
  { IDYES, 0x02000705 },
  { IDC_BUTTON_OVERWRITE_YES_TO_ALL, 0x02000707 },
  { IDNO,  0x02000709 },
  { IDC_BUTTON_OVERWRITE_NO_TO_ALL,0x0200070B },
  { IDC_BUTTON_OVERWRITE_AUTO_RENAME, 0x02000911 },
  { IDCANCEL, 0x02000711 }
};
#endif

static const int kCurrentFileNameSizeLimit = 82;
static const int kCurrentFileNameSizeLimit2 = 30;

void COverwriteDialog::ReduceString(UString &s)
{
  int size = _isBig ? kCurrentFileNameSizeLimit : kCurrentFileNameSizeLimit2;
  if (s.Length() > size)
    s = s.Left(size / 2) + UString(L" ... ") + s.Right(size / 2);
}

void COverwriteDialog::SetFileInfoControl(int textID, int iconID,
    const NOverwriteDialog::CFileInfo &fileInfo)
{
  UString sizeString;
  if (fileInfo.SizeIsDefined)
    sizeString = MyFormatNew(IDS_FILE_SIZE,
        #ifdef LANG
        0x02000982,
        #endif
        NumberToString(fileInfo.Size));

  const UString &fileName = fileInfo.Name;
  int slashPos = fileName.ReverseFind(WCHAR_PATH_SEPARATOR);
  UString s1, s2;
  if (slashPos >= 0)
  {
    s1 = fileName.Left(slashPos + 1);
    s2 = fileName.Mid(slashPos + 1);
  }
  else
    s2 = fileName;
  ReduceString(s1);
  ReduceString(s2);
  
  UString fullString = s1 + L'\n' + s2;
  fullString += L'\n';
  fullString += sizeString;
  fullString += L'\n';

  if (fileInfo.TimeIsDefined)
  {
    UString timeString;
    FILETIME localFileTime;
    if (!FileTimeToLocalFileTime(&fileInfo.Time, &localFileTime))
      throw 4190402;
    timeString = ConvertFileTimeToString(localFileTime);

    fullString +=
    #ifdef LANG
    LangString(IDS_FILE_MODIFIED, 0x02000983);
    #else
    MyLoadStringW(IDS_FILE_MODIFIED);
    #endif

    fullString += L" ";
    fullString += timeString;
  }

  NWindows::NControl::CDialogChildControl control;
  control.Init(*this, textID);
  control.SetText(fullString);

  SHFILEINFO shellFileInfo;
  if (::SHGetFileInfo(
      GetSystemString(fileInfo.Name), FILE_ATTRIBUTE_NORMAL, &shellFileInfo,
      sizeof(shellFileInfo), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES | SHGFI_LARGEICON))
  {
    NControl::CStatic staticContol;
    staticContol.Attach(GetItem(iconID));
    staticContol.SetIcon(shellFileInfo.hIcon);
  }
}

bool COverwriteDialog::OnInit()
{
  #ifdef LANG
  LangSetWindowText(HWND(*this), 0x02000900);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  SetFileInfoControl(IDC_STATIC_OVERWRITE_OLD_FILE_SIZE_TIME, IDC_STATIC_OVERWRITE_OLD_FILE_ICON, OldFileInfo);
  SetFileInfoControl(IDC_STATIC_OVERWRITE_NEW_FILE_SIZE_TIME, IDC_STATIC_OVERWRITE_NEW_FILE_ICON, NewFileInfo);
  NormalizePosition();
  return CModalDialog::OnInit();
}

bool COverwriteDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDYES:
    case IDC_BUTTON_OVERWRITE_YES_TO_ALL:
    case IDNO:
    case IDC_BUTTON_OVERWRITE_NO_TO_ALL:
    case IDC_BUTTON_OVERWRITE_AUTO_RENAME:
      End(buttonID);
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}
