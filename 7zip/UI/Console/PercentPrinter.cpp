// PercentPrinter.cpp

#include "StdAfx.h"

#include "Common/StdOutStream.h"
#include "Common/IntToString.h"

#include "PercentPrinter.h"

static const char *kPrepareString = "    ";
static const char *kCloseString = "\b\b\b\b    \b\b\b\b";
// static const char *kPercentFormatString =  "\b\b\b\b%3I64u%%";
static const char *kPercentFormatString1 = "\b\b\b\b";
static const int kNumDigits = 3;

CPercentPrinter::CPercentPrinter(UINT64 minStepSize):
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
    g_StdOut << (m_Spaces + m_ScreenPos);
  m_ScreenPos  = kNumPercentSpaces;
  g_StdOut << kPrepareString;
}

void CPercentPrinter::ClosePrint()
{
  g_StdOut << kCloseString;
  m_StringIsPrinted = false;
}

void CPercentPrinter::PrintString(const char *aString)
{
  m_ScreenPos += lstrlenA(aString);
  g_StdOut << aString;
}

void CPercentPrinter::PrintNewLine()
{
  m_ScreenPos = 0;
  g_StdOut << "\n";
  m_StringIsPrinted = false;
}

void CPercentPrinter::SetRatio(UINT64 doneValue)
  { m_CurValue = doneValue; }

void CPercentPrinter::RePrintRatio()
{
  if (m_Total == 0)
    return;
  UINT64 ratio = m_CurValue * 100 / m_Total;
  // char temp[32];
  // sprintf(temp, kPercentFormatString, ratio);
  char temp[32 + kNumDigits] = "    "; // for 4 digits;
  ConvertUINT64ToString(ratio, temp + kNumDigits);
  int len = lstrlenA(temp + kNumDigits);
  lstrcatA(temp, "%");
  int pos = (len > kNumDigits)? kNumDigits : len;
  g_StdOut << kPercentFormatString1;
  g_StdOut << (temp + pos);
  m_PrevValue = m_CurValue;
  m_StringIsPrinted = true;
}

void CPercentPrinter::PrintRatio()
{
  if (m_CurValue < m_PrevValue + m_MinStepSize || !m_StringIsPrinted)
    return;
  RePrintRatio();
}
