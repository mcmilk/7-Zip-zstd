// PercentPrinter.cpp

#include "StdAfx.h"

#include "PercentPrinter.h"
#include "Common/StdOutStream.h"

static const char *kPrepareString = "    ";
static const char *kCloseString = "\b\b\b\b    \b\b\b\b";
static const char *kPercentFormatString =  "\b\b\b\b%3I64u%%";

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
  m_ScreenPos += strlen(aString);
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
  char temp[32];
  UINT64 ratio = m_CurValue * 100 / m_Total;
  sprintf(temp, kPercentFormatString, ratio);
  g_StdOut << temp;
  m_PrevValue = m_CurValue;
  m_StringIsPrinted = true;
}

void CPercentPrinter::PrintRatio()
{
  if (m_CurValue < m_PrevValue + m_MinStepSize || !m_StringIsPrinted)
    return;
  RePrintRatio();
}
