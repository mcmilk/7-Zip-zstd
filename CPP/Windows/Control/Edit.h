// Windows/Control/Edit.h

#ifndef __WINDOWS_CONTROL_EDIT_H
#define __WINDOWS_CONTROL_EDIT_H

#include "Windows/Window.h"

namespace NWindows {
namespace NControl {

class CEdit: public CWindow
{
public:
  void SetPasswordChar(WPARAM c) { SendMessage(EM_SETPASSWORDCHAR, c); }
};

}}

#endif
