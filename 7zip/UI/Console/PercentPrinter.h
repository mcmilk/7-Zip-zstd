// PercentPrinter.h

#ifndef __PERCENTPRINTER_H
#define __PERCENTPRINTER_H

#include "Common/Defs.h"
#include "Common/Types.h"
#include "Common/StdOutStream.h"

const int kNumPercentSpaces = 70;
class CPercentPrinter
{
  UInt64 m_MinStepSize;
  UInt64 m_PrevValue;
  UInt64 m_CurValue;
  UInt64 m_Total;
  UInt32 m_ScreenPos;
  char m_Spaces[kNumPercentSpaces + 1];
  bool m_StringIsPrinted;
public:
  CStdOutStream *OutStream;

  CPercentPrinter(UInt64 minStepSize = 1);
  void SetTotal(UInt64 total)
  {
    m_Total = total;
    m_PrevValue = 0;
    m_StringIsPrinted = false;
  }
  void PrintString(const char *s);
  void PrintString(const wchar_t *s);
  void PrintNewLine();
  void PreparePrint();
  void ClosePrint();
  void SetRatio(UInt64 doneValue);
  void RePrintRatio();
  void PrintRatio();
};

#endif
