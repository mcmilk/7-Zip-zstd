// Windows/Control/Static.h

#pragma once

#ifndef __WINDOWS_CONTROL_STATIC_H
#define __WINDOWS_CONTROL_STATIC_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CStatic: public CWindow
{
public:
  HICON SetIcon(HICON anIcon)
    { return (HICON)SendMessage(STM_SETICON, (WPARAM)anIcon, 0); }
  HICON GetIcon()
    { return (HICON)SendMessage(STM_GETICON, 0, 0); }
  HANDLE SetImage(WPARAM anImageType, HANDLE aHandle)
    { return (HANDLE)SendMessage(STM_SETIMAGE, anImageType, (LPARAM)aHandle); }
  HANDLE GetImage(WPARAM anImageType)
    { return (HANDLE)SendMessage(STM_GETIMAGE, anImageType, 0); }
};

}}

#endif