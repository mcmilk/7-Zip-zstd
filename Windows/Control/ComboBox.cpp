// Windows/Control/ComboBox.cpp

#include "StdAfx.h"

#include "Windows/Control/ComboBox.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

int CComboBox::GetLBText(int index, CSysString &string)
{
  string.Empty();
  int aLength = GetLBTextLen(index);
  if (aLength == CB_ERR)
    return aLength;
  aLength = GetLBText(index, string.GetBuffer(aLength));
  string.ReleaseBuffer();
  return aLength;
}


}}
