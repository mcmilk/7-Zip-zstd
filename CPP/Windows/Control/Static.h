// Windows/Control/Static.h

#ifndef __WINDOWS_CONTROL_STATIC_H
#define __WINDOWS_CONTROL_STATIC_H

#include "../Window.h"

namespace NWindows {
namespace NControl {

class CStatic: public CWindow
{
public:
  HANDLE SetImage(WPARAM imageType, HANDLE handle) { return (HANDLE)SendMessage(STM_SETIMAGE, imageType, (LPARAM)handle); }
  HANDLE GetImage(WPARAM imageType) { return (HANDLE)SendMessage(STM_GETIMAGE, imageType, 0); }

  #ifdef UNDER_CE
  HICON SetIcon(HICON icon) { return (HICON)SetImage(IMAGE_ICON, icon); }
  HICON GetIcon() { return (HICON)GetImage(IMAGE_ICON); }
  #else
  HICON SetIcon(HICON icon) { return (HICON)SendMessage(STM_SETICON, (WPARAM)icon, 0); }
  HICON GetIcon() { return (HICON)SendMessage(STM_GETICON, 0, 0); }
  #endif
};

}}

#endif
