// Windows/Control/StatusBar.h
  
#pragma once

#ifndef __WINDOWS_CONTROL_STATUSBAR_H
#define __WINDOWS_CONTROL_STATUSBAR_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CStatusBar: public NWindows::CWindow
{
public:
  bool Create(LONG style, LPCTSTR text, HWND hwndParent, UINT id)
    { return (_window = ::CreateStatusWindow(style, text, hwndParent, id)) != 0; }
  bool SetParts(int numParts, const int *edgePostions)
    { return BOOLToBool(SendMessage(SB_SETPARTS, numParts, (LPARAM)edgePostions)); }
  bool SetText(LPCTSTR text)
    { return CWindow::SetText(text); }
  bool SetText(int index, LPCTSTR text, UINT type)
    { return BOOLToBool(SendMessage(SB_SETTEXT, index | type, (LPARAM)text)); }
  bool SetText(int index, LPCTSTR text)
    { return SetText(index, text, 0); }
  void Simple(bool simple)
    { SendMessage(SB_SIMPLE, BoolToBOOL(simple), 0); }
};

}}

#endif