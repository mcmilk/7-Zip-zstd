// Windows/Control/ComboBox.cpp

#include "StdAfx.h"

#include "Windows/Control/ComboBox.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

int CComboBox::GetLBText(int anIndex, CSysString &aString)
{
  aString.Empty();
  int aLength = GetLBTextLen(anIndex);
  if (aLength == CB_ERR)
    return aLength;
  aLength = GetLBText(anIndex, aString.GetBuffer(aLength));
  aString.ReleaseBuffer();
  return aLength;
}


}}
