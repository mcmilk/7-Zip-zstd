// Windows/Control/ProgressBar.h

#pragma once

#ifndef __WINDOWS_CONTROL_PROGRESSBAR_H
#define __WINDOWS_CONTROL_PROGRESSBAR_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CProgressBar: public CWindow
{
public:
  LRESULT SetPos(int aPos)
    { return SendMessage(PBM_SETPOS, aPos, 0); }
  LRESULT DeltaPos(int anIncrement)
    { return SendMessage(PBM_DELTAPOS, anIncrement, 0); }
  UINT GetPos()
    { return SendMessage(PBM_GETPOS, 0, 0); }
  LRESULT SetRange(unsigned short aMinValue, unsigned short aMaxValue)
    { return SendMessage(PBM_SETRANGE, 0, MAKELPARAM(aMinValue, aMaxValue)); }
  DWORD SetRange32(int aMinValue, int aMaxValue)
    { return SendMessage(PBM_SETRANGE32, aMinValue, aMaxValue); }
  int SetStep(int aStep)
    { return SendMessage(PBM_SETSTEP, aStep, 0); }
  int StepIt()
    { return SendMessage(PBM_STEPIT, 0, 0); }

  int GetRange(bool aMinValue, PPBRANGE aRange)
    { return SendMessage(PBM_GETRANGE, BoolToBOOL(aMinValue), (LPARAM)aRange); }
  
  COLORREF SetBarColor(COLORREF aColor)
    { return SendMessage(PBM_SETBARCOLOR, 0, aColor); }
  COLORREF SetBackgroundColor(COLORREF aColor)
    { return SendMessage(PBM_SETBKCOLOR, 0, aColor); }
};

}}

#endif