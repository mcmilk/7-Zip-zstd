// ProgressBox.cpp

#include "StdAfx.h"

#include "ProgressBox.h"

#include "FarUtils.h"

using namespace NFar;

static void CopySpaces(char *aString, int aNumSpaces)
{
  for(int i = 0; i < aNumSpaces; i++)
    aString[i] = ' ';
  aString[i] = '\0';
}

/////////////////////////////////
// CMessageBox

const kNumStringsMax = 10;

void CMessageBox::Init(const CSysString &aTitle, const CSysString &aMessage, 
    int aNumStrings, int aWidth)
{
  if (aNumStrings > kNumStringsMax)
    throw 120620;
  m_NumStrings = aNumStrings;
  m_Width = aWidth;

  m_Title = aTitle;
  m_Message = aMessage;
}

const int kNumStaticStrings = 2;

void CMessageBox::ShowProcessMessages(const char *aMessages[])
{
  const char *aMsgItems[kNumStaticStrings + kNumStringsMax];
  aMsgItems[0] = m_Title;
  aMsgItems[1] = m_Message;

  char aFormattedMessages[kNumStringsMax][256];

  for (int i = 0; i < m_NumStrings; i++)
  {
    char *aFormattedMessage = aFormattedMessages[i];
    int aLen = strlen(aMessages[i]);
    int aSize = MyMax(m_Width, aLen);
    int aStartPos = (aSize - aLen) / 2;
    CopySpaces(aFormattedMessage, aStartPos);
    strcpy(aFormattedMessage + aStartPos, aMessages[i]);
    CopySpaces(aFormattedMessage + aStartPos + aLen, aSize - aStartPos - aLen);
    aMsgItems[kNumStaticStrings + i] = aFormattedMessage;
  }

  g_StartupInfo.ShowMessage(0, NULL, aMsgItems, kNumStaticStrings + m_NumStrings, 0);
}

/////////////////////////////////
// CProgressBox

void CProgressBox::Init(const CSysString &aTitle, const CSysString &aMessage,
    UINT64 aStep)
{
  CMessageBox::Init(aTitle, aMessage, 1, 22);
  m_Step = aStep;
  m_CompletedPrev = 0;
  m_Total = 0;
}


void CProgressBox::ShowProcessMessage(const char *aMessage)
{
  CMessageBox::ShowProcessMessages(&aMessage);
}

void CProgressBox::PrintPercent(UINT64 aPercent)
{
  char aValueBuffer[32];
  sprintf(aValueBuffer, "%I64u%%", aPercent);
  ShowProcessMessage(aValueBuffer);
}

void CProgressBox::SetTotal(UINT64 aTotal)
{
  m_Total = aTotal;
}

void CProgressBox::PrintCompeteValue(UINT64 aCompleted)
{
  if (aCompleted >= m_CompletedPrev + m_Step || aCompleted < m_CompletedPrev ||
      aCompleted == 0)
  {
    if (m_Total == 0)
      PrintPercent(0);
    else
      PrintPercent(aCompleted * 100 / m_Total);
    m_CompletedPrev = aCompleted;
  }
}
