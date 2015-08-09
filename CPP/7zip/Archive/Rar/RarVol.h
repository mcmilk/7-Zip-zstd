// RarVol.h

#ifndef __ARCHIVE_RAR_VOL_H
#define __ARCHIVE_RAR_VOL_H

#include "../../../Common/StringConvert.h"

#include "RarHeader.h"

namespace NArchive {
namespace NRar {

inline bool IsDigit(wchar_t c)
{
  return c >= L'0' && c <= L'9';
}

class CVolumeName
{
  bool _first;
  bool _newStyle;
  UString _unchangedPart;
  UString _changedPart;
  UString _afterPart;
public:
  CVolumeName(): _newStyle(true) {};

  bool InitName(const UString &name, bool newStyle = true)
  {
    _first = true;
    _newStyle = newStyle;
    int dotPos = name.ReverseFind_Dot();
    UString basePart = name;

    if (dotPos >= 0)
    {
      UString ext = name.Ptr(dotPos + 1);
      if (ext.IsEqualTo_Ascii_NoCase("rar"))
      {
        _afterPart = name.Ptr(dotPos);
        basePart = name.Left(dotPos);
      }
      else if (ext.IsEqualTo_Ascii_NoCase("exe"))
      {
        _afterPart.SetFromAscii(".rar");
        basePart = name.Left(dotPos);
      }
      else if (!_newStyle)
      {
        if (ext.IsEqualTo_Ascii_NoCase("000") ||
            ext.IsEqualTo_Ascii_NoCase("001") ||
            ext.IsEqualTo_Ascii_NoCase("r00") ||
            ext.IsEqualTo_Ascii_NoCase("r01"))
        {
          _afterPart.Empty();
          _first = false;
          _changedPart = ext;
          _unchangedPart = name.Left(dotPos + 1);
          return true;
        }
      }
    }

    if (!_newStyle)
    {
      _afterPart.Empty();
      _unchangedPart = basePart;
      _unchangedPart += L'.';
      _changedPart.SetFromAscii("r00");
      return true;
    }

    if (basePart.IsEmpty())
      return false;
    unsigned i = basePart.Len();
    
    do
      if (!IsDigit(basePart[i - 1]))
        break;
    while (--i);
    
    _unchangedPart = basePart.Left(i);
    _changedPart = basePart.Ptr(i);
    return true;
  }

  /*
  void MakeBeforeFirstName()
  {
    unsigned len = _changedPart.Len();
    _changedPart.Empty();
    for (unsigned i = 0; i < len; i++)
      _changedPart += L'0';
  }
  */

  UString GetNextName()
  {
    if (_newStyle || !_first)
    {
      unsigned i = _changedPart.Len();
      for (;;)
      {
        wchar_t c = _changedPart[--i];
        if (c == L'9')
        {
          c = L'0';
          _changedPart.ReplaceOneCharAtPos(i, c);
          if (i == 0)
          {
            _changedPart.InsertAtFront(L'1');
            break;
          }
          continue;
        }
        c++;
        _changedPart.ReplaceOneCharAtPos(i, c);
        break;
      }
    }
    
    _first = false;
    return _unchangedPart + _changedPart + _afterPart;
  }
};

}}

#endif
