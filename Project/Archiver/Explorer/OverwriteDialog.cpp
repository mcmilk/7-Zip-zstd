// OverwriteDialog.cpp

#include "StdAfx.h"

#include "OverwriteDialog.h"

#include "Windows/NationalTime.h"
#include "Windows/FileName.h"
#include "Windows/Defs.h"
#include "Windows/ResourceString.h"
#include "Windows/Control/Static.h"

#include "FormatUtils.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
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

void ConvertFileTimeToStrings(const FILETIME &aFileTime, CSysString &aDateString,
    CSysString &aTimeString)
{
  SYSTEMTIME aSystemTime;
  if(!BOOLToBool(FileTimeToSystemTime(&aFileTime, &aSystemTime)))
    throw 311907;
  const kBufferSize = 64;
  if(!NNational::NTime::MyGetDateFormat(LOCALE_USER_DEFAULT, 
      DATE_LONGDATE, &aSystemTime, NULL, aDateString))
    throw 311908;
  
  if(!NNational::NTime::MyGetTimeFormat(LOCALE_USER_DEFAULT, 
      0, &aSystemTime, NULL, aTimeString))
    throw 311909;
}
 
void COverwriteDialog::SetFileInfoControl(int aTextID, int anIconID, 
    const NOverwriteDialog::CFileInfo &aFileInfo) 
{
  CSysString aSizeString;
  if (aFileInfo.SizeIsDefined)
    aSizeString = MyFormat(IDS_FILE_SIZE, 
        #ifdef LANG        
        0x02000982, 
        #endif
        NumberToString(aFileInfo.Size));

  CSysString aDateString, aTimeString;
  FILETIME aLocalFileTime; 
  if (!FileTimeToLocalFileTime(&aFileInfo.Time, &aLocalFileTime))
    throw 4190402;
  ConvertFileTimeToStrings(aLocalFileTime, aDateString, aTimeString);
  TCHAR sz[512];
  
  CSysString aReducedName;
  const kLineSize = 88;
  for (int i = 0; i < aFileInfo.Name.Length();)
  {
    aReducedName += aFileInfo.Name.Mid(i, kLineSize);
    aReducedName += TEXT(" ");
    i += kLineSize;
  }

  _stprintf(sz, 
      #ifdef LANG
      LangLoadString(IDS_FILE_SIZE_TIME, 0x02000981),
      #else
      MyLoadString(IDS_FILE_SIZE_TIME), 
      #endif
      (LPCTSTR)aReducedName, (LPCTSTR)aSizeString, 
      (LPCTSTR)aDateString, (LPCTSTR)aTimeString);
  NWindows::NControl::CDialogChildControl m_Control;
  m_Control.Init(*this, aTextID);
  m_Control.SetText(sz);

  SHFILEINFO aShellFileInfo;
  if (::SHGetFileInfo(aFileInfo.Name, FILE_ATTRIBUTE_NORMAL, &aShellFileInfo, 
      sizeof(aShellFileInfo), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES | SHGFI_LARGEICON)) 
  {
    NControl::CStatic aStatic;
    aStatic.Attach(GetItem(anIconID));
    aStatic.SetIcon(aShellFileInfo.hIcon);
  }
}

bool COverwriteDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000900);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  SetFileInfoControl(IDC_STATIC_OVERWRITE_OLD_FILE_SIZE_TIME, 
      IDC_STATIC_OVERWRITE_OLD_FILE_ICON, m_OldFileInfo);
  SetFileInfoControl(IDC_STATIC_OVERWRITE_NEW_FILE_SIZE_TIME, 
      IDC_STATIC_OVERWRITE_NEW_FILE_ICON, m_NewFileInfo);
	return CModalDialog::OnInit();
}

bool COverwriteDialog::OnButtonClicked(int aButtonID, HWND aButtonHWND) 
{ 
  switch(aButtonID)
  {
    case IDYES:
    case IDC_BUTTON_OVERWRITE_YES_TO_ALL:
    case IDNO:
    case IDC_BUTTON_OVERWRITE_NO_TO_ALL:
    case IDC_BUTTON_OVERWRITE_AUTO_RENAME:
      End(aButtonID);
      return true;
  }
  return CModalDialog::OnButtonClicked(aButtonID, aButtonHWND);
}
