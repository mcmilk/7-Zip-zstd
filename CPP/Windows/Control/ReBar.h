// Windows/Control/ReBar.h
  
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
    { return LRESULTToBool(SendMessage(RB_SETBARINFO, 0, (LPARAM)barInfo)); }
  bool InsertBand(int index, LPREBARBANDINFO bandInfo)
    { return LRESULTToBool(SendMessage(RB_INSERTBAND, index, (LPARAM)bandInfo)); }
  bool SetBandInfo(int index, LPREBARBANDINFO bandInfo)
    { return LRESULTToBool(SendMessage(RB_SETBANDINFO, index, (LPARAM)bandInfo)); }
  void MaximizeBand(int index, bool ideal)
    { SendMessage(RB_MAXIMIZEBAND, index, BoolToBOOL(ideal)); }
  bool SizeToRect(LPRECT rect)
    { return LRESULTToBool(SendMessage(RB_SIZETORECT, 0, (LPARAM)rect)); }
  UINT GetHeight()
    { return (UINT)SendMessage(RB_GETBARHEIGHT); }
  UINT GetBandCount()
    { return (UINT)SendMessage(RB_GETBANDCOUNT); }
  bool DeleteBand(UINT index)
    { return LRESULTToBool(SendMessage(RB_DELETEBAND, index)); }
};

}}

#endif
