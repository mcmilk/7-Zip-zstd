// Windows/Control/Static.h

#ifndef __WINDOWS_CONTROL_STATIC_H
#define __WINDOWS_CONTROL_STATIC_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CStatic: public CWindow
{
public:
  HICON SetIcon(HICON icon)
    { return (HICON)SendMessage(STM_SETICON, (WPARAM)icon, 0); }
  HICON GetIcon()
    { return (HICON)SendMessage(STM_GETICON, 0, 0); }
  HANDLE SetImage(WPARAM imageType, HANDLE handle)
    { return (HANDLE)SendMessage(STM_SETIMAGE, imageType, (LPARAM)handle); }
  HANDLE GetImage(WPARAM imageType)
    { return (HANDLE)SendMessage(STM_GETIMAGE, imageType, 0); }
};

}}

#endif