// ProgressBox.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "ProgressBox.h"

#include "FarUtils.h"

using namespace NFar;

static void CopySpaces(char *destString, int numSpaces)
{
  int i;
  for(i = 0; i < numSpaces; i++)
    destString[i] = ' ';
  destString[i] = '\0';
}

/////////////////////////////////
// CMessageBox

const int kNumStringsMax = 10;

void CMessageBox::Init(const CSysString &title, const CSysString &message, 
    int numStrings, int width)
{
  if (numStrings > kNumStringsMax)
    throw 120620;
  m_NumStrings = numStrings;
  m_Width = width;

  m_Title = title;
  m_Message = message;
}

const int kNumStaticStrings = 2;

void CMessageBox::ShowProcessMessages(const char *messages[])
{
  const char *msgItems[kNumStaticStrings + kNumStringsMax];
  msgItems[0] = m_Title;
  msgItems[1] = m_Message;

  char formattedMessages[kNumStringsMax][256];

  for (int i = 0; i < m_NumStrings; i++)
  {
    char *formattedMessage = formattedMessages[i];
    int len = (int)strlen(messages[i]);
    int size = MyMax(m_Width, len);
    int startPos = (size - len) / 2;
    CopySpaces(formattedMessage, startPos);
    MyStringCopy(formattedMessage + startPos, messages[i]);
    CopySpaces(formattedMessage + startPos + len, size - startPos - len);
    msgItems[kNumStaticStrings + i] = formattedMessage;
  }

  g_StartupInfo.ShowMessage(0, NULL, msgItems, kNumStaticStrings + m_NumStrings, 0);
}

/////////////////////////////////
// CProgressBox

void CProgressBox::Init(const CSysString &title, const CSysString &message,
    UInt64 step)
{
  CMessageBox::Init(title, message, 1, 22);
  m_Step = step;
  m_CompletedPrev = 0;
  m_Total = 0;
}


void CProgressBox::ShowProcessMessage(const char *message)
{
  CMessageBox::ShowProcessMessages(&message);
}

void CProgressBox::PrintPercent(UInt64 percent)
{
  char valueBuffer[32];
  sprintf(valueBuffer, "%I64u%%", percent);
  ShowProcessMessage(valueBuffer);
}

void CProgressBox::SetTotal(UInt64 total)
{
  m_Total = total;
}

void CProgressBox::PrintCompeteValue(UInt64 completed)
{
  if (completed >= m_CompletedPrev + m_Step || completed < m_CompletedPrev ||
      completed == 0)
  {
    if (m_Total == 0)
      PrintPercent(0);
    else
      PrintPercent(completed * 100 / m_Total);
    m_CompletedPrev = completed;
  }
}
