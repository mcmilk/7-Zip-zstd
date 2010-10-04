// Windows/Control/Trackbar.h

#ifndef __WINDOWS_CONTROL_TRACKBAR_H
#define __WINDOWS_CONTROL_TRACKBAR_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CTrackbar: public CWindow
{
public:
  void SetRange(int minimum, int maximum, bool redraw = true)
    { SendMessage(TBM_SETRANGE, BoolToBOOL(redraw), MAKELONG(minimum, maximum)); }
  void SetPos(int pos, bool redraw = true)
    { SendMessage(TBM_SETPOS, BoolToBOOL(redraw), pos); }
  void SetTicFreq(int freq)
    { SendMessage(TBM_SETTICFREQ, freq); }
  
  int GetPos()
    { return (int)SendMessage(TBM_GETPOS); }
};

}}

#endif
