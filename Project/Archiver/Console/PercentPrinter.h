// PercentPrinter.h

#pragma once

#ifndef __PERCENTPRINTER_H
#define __PERCENTPRINTER_H

#include "Common/Defs.h"

const kNumPercentSpaces = 64;
class CPercentPrinter
{
  UINT64 m_MinStepSize;
  UINT64 m_PrevValue;
  UINT64 m_CurValue;
  UINT64 m_Total;
  UINT32 m_ScreenPos;
  char m_Spaces[kNumPercentSpaces + 1];
  bool m_StringIsPrinted;
public:
  CPercentPrinter(UINT64 aMinStepSize = 1);
  void SetTotal(UINT64 aTotal)
  {
    m_Total = aTotal;
    m_PrevValue = 0;
    m_StringIsPrinted = false;
  }
  void PrintString(const char *aString);
  void PrintNewLine();
  void PreparePrint();
  void ClosePrint();
  void SetRatio(UINT64 aDoneValue);
  void RePrintRatio();
  void PrintRatio();
};

#endif
