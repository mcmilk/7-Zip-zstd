// EditPage.cpp

#include "StdAfx.h"
#include "resource.h"
#include "EditPage.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
// #include "Windows/FileFind.h"
// #include "Windows/FileDir.h"

#include "../../RegistryUtils.h"
#include "../../HelpUtils.h"
#include "../../LangUtils.h"
#include "../../ProgramLocation.h"

using namespace NWindows;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_EDIT_STATIC_EDITOR, 0x03010201}
};

static LPCWSTR kEditTopic = L"FM/options.htm#editor";

bool CEditPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  _editorEdit.Attach(GetItem(IDC_EDIT_EDIT_EDITOR));
  CSysString editorPath;
  ReadRegEditor(editorPath);
  _editorEdit.SetText(editorPath);
  return CPropertyPage::OnInit();
}

LONG CEditPage::OnApply()
{
  // int selectedIndex = _langCombo.GetCurSel();
  // int pathIndex = _langCombo.GetItemData(selectedIndex);
  // ReloadLang();
  CSysString editorPath;
  _editorEdit.GetText(editorPath);
  SaveRegEditor(editorPath);
  return PSNRET_NOERROR;
}

void CEditPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kEditTopic); // change it
}

bool CEditPage::OnButtonClicked(int aButtonID, HWND aButtonHWND)
{
  switch(aButtonID)
  {
    case IDC_EDIT_BUTTON_SET:
    {
      OnSetEditorButton();
      // if (!NShell::BrowseForFolder(HWND(*this), title, currentPath, aResultPath))
      //   return;
      return true;
    }
  }
  return CPropertyPage::OnButtonClicked(aButtonID, aButtonHWND);
}

class CDoubleZeroStringList
{
  CRecordVector<int> _indexes;
  CSysString _string;
public:
  void Add(LPCTSTR string);
  void SetForBuffer(LPTSTR buffer);
};

const TCHAR kDelimiterSymbol = TEXT(' ');
void CDoubleZeroStringList::Add(LPCTSTR string)
{
  _string += string;
  _indexes.Add(_string.Length());
  _string += kDelimiterSymbol;
}

void CDoubleZeroStringList::SetForBuffer(LPTSTR buffer)
{
  lstrcpy(buffer, _string);
  for (int i = 0; i < _indexes.Size(); i++)
    buffer[_indexes[i]] = TEXT('\0');
}

void CEditPage::OnSetEditorButton()
{
  OPENFILENAME info;
  info.lStructSize = sizeof(info); 
  info.hwndOwner = HWND(*this); 
  info.hInstance = 0; 
  
  const int kBufferSize = MAX_PATH * 2;
  TCHAR buffer[kBufferSize + 1];
  CSysString editorPath;
  _editorEdit.GetText(editorPath);

  lstrcpy(buffer, editorPath);

  const int kFilterBufferSize = MAX_PATH;
  TCHAR filterBuffer[kFilterBufferSize];
  CDoubleZeroStringList doubleZeroStringList;
  CSysString string = TEXT("*.exe");
  doubleZeroStringList.Add(string);
  doubleZeroStringList.Add(string);
  doubleZeroStringList.SetForBuffer(filterBuffer);
  info.lpstrFilter = filterBuffer; 
  
  info.lpstrCustomFilter = NULL; 
  info.nMaxCustFilter = 0; 
  info.nFilterIndex = 0; 
  
  info.lpstrFile = buffer; 
  info.nMaxFile = kBufferSize;
  
  info.lpstrFileTitle = NULL; 
  info.nMaxFileTitle = 0; 
  
  info.lpstrInitialDir= NULL; 

  /*
  CSysString title = "Open";
    LangLoadString(IDS_COMPRESS_SET_ARCHIVE_DIALOG_TITLE, 0x02000D90);
  info.lpstrTitle = title;
  */
  info.lpstrTitle = 0;
 

  info.Flags = OFN_EXPLORER | OFN_HIDEREADONLY; 
  info.nFileOffset = 0; 
  info.nFileExtension = 0; 
  info.lpstrDefExt = NULL; 
  
  info.lCustData = 0; 
  info.lpfnHook = NULL; 
  info.lpTemplateName = NULL; 

  if(!GetOpenFileName(&info))
    return;
  _editorEdit.SetText(buffer);
  // Changed();
}

bool CEditPage::OnCommand(int code, int itemID, LPARAM param)
{
  if (code == EN_CHANGE && itemID == IDC_EDIT_EDIT_EDITOR)
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}


