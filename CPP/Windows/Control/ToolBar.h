// Windows/Control/ToolBar.h
  
#ifndef __WINDOWS_CONTROL_TOOLBAR_H
#define __WINDOWS_CONTROL_TOOLBAR_H

#include "Windows/Window.h"

namespace NWindows {
namespace NControl {

class CToolBar: public NWindows::CWindow
{
public:
  void AutoSize() { SendMessage(TB_AUTOSIZE, 0, 0); }
  DWORD GetButtonSize() { return (DWORD)SendMessage(TB_GETBUTTONSIZE, 0, 0); }
  
  bool GetMaxSize(LPSIZE size)
  #ifdef UNDER_CE
  {
    // maybe it must be fixed for more than 1 buttons
    DWORD val = GetButtonSize();
    size->cx = LOWORD(val);
    size->cy = HIWORD(val);
    return true;
  }
  #else
  {
    return LRESULTToBool(SendMessage(TB_GETMAXSIZE, 0, (LPARAM)size));
  }
  #endif

  bool EnableButton(UINT buttonID, bool enable) { return LRESULTToBool(SendMessage(TB_ENABLEBUTTON, buttonID, MAKELONG(BoolToBOOL(enable), 0))); }
  void ButtonStructSize() { SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON)); }
  HIMAGELIST SetImageList(UINT listIndex, HIMAGELIST imageList) { return HIMAGELIST(SendMessage(TB_SETIMAGELIST, listIndex, (LPARAM)imageList)); }
  bool AddButton(UINT numButtons, LPTBBUTTON buttons) { return LRESULTToBool(SendMessage(TB_ADDBUTTONS, numButtons, (LPARAM)buttons)); }
  #ifndef _UNICODE
  bool AddButtonW(UINT numButtons, LPTBBUTTON buttons) { return LRESULTToBool(SendMessage(TB_ADDBUTTONSW, numButtons, (LPARAM)buttons)); }
  #endif
};

}}

#endif
