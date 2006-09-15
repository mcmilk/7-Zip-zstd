// Windows/Control/ProgressBar.h

#ifndef __WINDOWS_CONTROL_PROGRESSBAR_H
#define __WINDOWS_CONTROL_PROGRESSBAR_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CProgressBar: public CWindow
{
public:
  LRESULT SetPos(int pos)
    { return SendMessage(PBM_SETPOS, pos, 0); }
  LRESULT DeltaPos(int increment)
    { return SendMessage(PBM_DELTAPOS, increment, 0); }
  UINT GetPos()
    { return (UINT)SendMessage(PBM_GETPOS, 0, 0); }
  LRESULT SetRange(unsigned short minValue, unsigned short maxValue)
    { return SendMessage(PBM_SETRANGE, 0, MAKELPARAM(minValue, maxValue)); }
  DWORD SetRange32(int minValue, int maxValue)
    { return (DWORD)SendMessage(PBM_SETRANGE32, minValue, maxValue); }
  int SetStep(int step)
    { return (int)SendMessage(PBM_SETSTEP, step, 0); }
  LRESULT StepIt()
    { return SendMessage(PBM_STEPIT, 0, 0); }

  INT GetRange(bool minValue, PPBRANGE range)
    { return (INT)SendMessage(PBM_GETRANGE, BoolToBOOL(minValue), (LPARAM)range); }
  
  COLORREF SetBarColor(COLORREF color)
    { return (COLORREF)SendMessage(PBM_SETBARCOLOR, 0, color); }
  COLORREF SetBackgroundColor(COLORREF color)
    { return (COLORREF)SendMessage(PBM_SETBKCOLOR, 0, color); }
};

}}

#endif