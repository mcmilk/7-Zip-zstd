// Windows/Control/ReBar.h
  
#pragma once

#ifndef __WINDOWS_CONTROL_REBAR_H
#define __WINDOWS_CONTROL_REBAR_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CReBar: public NWindows::CWindow
{
public:
  bool SetBarInfo(LPREBARINFO barInfo)
    { return BOOLToBool(SendMessage(RB_SETBARINFO, 0, (LPARAM)barInfo)); }
  bool InsertBand(int index, LPREBARBANDINFO bandInfo)
    { return BOOLToBool(SendMessage(RB_INSERTBAND, index, (LPARAM)bandInfo)); }
  bool SetBandInfo(int index, LPREBARBANDINFO bandInfo)
    { return BOOLToBool(SendMessage(RB_SETBANDINFO, index, (LPARAM)bandInfo)); }
  void MaximizeBand(int index, bool ideal)
    { SendMessage(RB_MAXIMIZEBAND, index, BoolToBOOL(ideal)); }
};

}}

#endif