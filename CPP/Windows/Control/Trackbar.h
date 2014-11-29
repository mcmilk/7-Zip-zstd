// Windows/Control/Trackbar.h

#ifndef __WINDOWS_CONTROL_TRACKBAR_H
#define __WINDOWS_CONTROL_TRACKBAR_H

#include "../Window.h"
#include "../Defs.h"

namespace NWindows {
namespace NControl {

class CTrackbar1: public CWindow
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
