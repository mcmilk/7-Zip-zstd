// Windows/Control/ToolBar.h
  
#pragma once

#ifndef __WINDOWS_CONTROL_TOOLBAR_H
#define __WINDOWS_CONTROL_TOOLBAR_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CToolBar: public NWindows::CWindow
{
public:
  bool GetMaxSize(LPSIZE size)
    { return BOOLToBool(SendMessage(TB_GETMAXSIZE, 0, (LPARAM)size)); }
  bool EnableButton(UINT buttonID, bool enable)
    { return BOOLToBool(SendMessage(TB_ENABLEBUTTON, buttonID, 
          MAKELONG(BoolToBOOL(enable), 0))); }
};

}}

#endif