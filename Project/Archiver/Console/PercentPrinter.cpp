// PercentPrinter.cpp

#include "StdAfx.h"

#include "PercentPrinter.h"
#include "Common/StdOutStream.h"

static const DWORD kDecimalBaseAfterPoint = 10;
static const DWORD kRatioBase = 100 * kDecimalBaseAfterPoint;

static const char *kPrepareString = "      ";
static const char *kCloseString = "\b\b\b\b\b\b      \b\b\b\b\b\b";

static const char *kPercentFormatString =  "\b\b\b\b\b\b%3I64u.%1u%%";

const kPercentScreePos = 64;

CPercentPrinter::CPercentPrinter(UINT64 aMinStepSize):
  m_MinStepSize(aMinStepSize),
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

void CPercentPrinter::SetRatio(UINT64 aDoneValue)
  { m_CurValue = aDoneValue; }

void CPercentPrinter::RePrintRatio()
{
  if (m_Total == 0)
    return;
  char aString[32];
  UINT64 aRatio = m_CurValue * 1000 / m_Total;
  sprintf(aString, kPercentFormatString, aRatio / kDecimalBaseAfterPoint, 
      UINT32(aRatio % kDecimalBaseAfterPoint));
  g_StdOut << aString;
  m_PrevValue = m_CurValue;
  m_StringIsPrinted = true;
}

void CPercentPrinter::PrintRatio()
{
  if (m_CurValue < m_PrevValue + m_MinStepSize || !m_StringIsPrinted)
    return;
  RePrintRatio();
}
