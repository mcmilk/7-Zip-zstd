// PercentPrinter.cpp

#include "StdAfx.h"

#include "Common/StdOutStream.h"
#include "Common/IntToString.h"
#include "Common/String.h"

#include "PercentPrinter.h"

static const char *kPrepareString = "    ";
static const char *kCloseString = "\b\b\b\b    \b\b\b\b";
// static const char *kPercentFormatString =  "\b\b\b\b%3I64u%%";
static const char *kPercentFormatString1 = "\b\b\b\b";
static const int kNumDigits = 3;

CPercentPrinter::CPercentPrinter(UInt64 minStepSize):
  m_MinStepSize(minStepSize),
  m_ScreenPos(0),
  m_StringIsPrinted(false)
{
  for (int i = 0; i < kNumPercentSpaces; i++)
    m_Spaces[i] = ' ';
  m_Spaces[kNumPercentSpaces] = '\0';
}

void CPercentPrinter::PreparePrint()
{
  if (m_ScreenPos < kNumPercentSpaces)
    g_StdErr << (m_Spaces + m_ScreenPos);
  m_ScreenPos  = kNumPercentSpaces;
  g_StdErr << kPrepareString;
}

void CPercentPrinter::ClosePrint()
{
  g_StdErr << kCloseString;
  m_StringIsPrinted = false;
}

void CPercentPrinter::PrintString(const char *s)
{
  m_ScreenPos += MyStringLen(s);
  g_StdErr << s;
}

void CPercentPrinter::PrintString(const wchar_t *s)
{
  m_ScreenPos += MyStringLen(s);
  g_StdErr << s;
}

void CPercentPrinter::PrintNewLine()
{
  m_ScreenPos = 0;
  g_StdErr << "\n";
  m_StringIsPrinted = false;
}

void CPercentPrinter::SetRatio(UInt64 doneValue)
  { m_CurValue = doneValue; }

void CPercentPrinter::RePrintRatio()
{
  if (m_Total == 0)
    return;
  UInt64 ratio = m_CurValue * 100 / m_Total;
  // char temp[32];
  // sprintf(temp, kPercentFormatString, ratio);
  char temp[32 + kNumDigits] = "    "; // for 4 digits;
  ConvertUInt64ToString(ratio, temp + kNumDigits);
  int len = lstrlenA(temp + kNumDigits);
  lstrcatA(temp, "%");
  int pos = (len > kNumDigits)? kNumDigits : len;
  g_StdErr << kPercentFormatString1;
  g_StdErr << (temp + pos);
  m_PrevValue = m_CurValue;
  m_StringIsPrinted = true;
}

void CPercentPrinter::PrintRatio()
{
  if (m_CurValue < m_PrevValue + m_MinStepSize || !m_StringIsPrinted)
    return;
  RePrintRatio();
}
