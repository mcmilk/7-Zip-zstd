// ProgressBox.cpp

#include "StdAfx.h"

#include <stdio.h>

#include "ProgressBox.h"
#include "Common/IntToString.h"
#include "FarUtils.h"

static void CopySpaces(char *dest, int numSpaces)
{
  int i;
  for (i = 0; i < numSpaces; i++)
    dest[i] = ' ';
  dest[i] = '\0';
}

void ConvertUInt64ToStringAligned(UInt64 value, char *s, int alignSize)
{
  char temp[32];
  ConvertUInt64ToString(value, temp);
  int len = (int)strlen(temp);
  int numSpaces = 0;
  if (len < alignSize)
  {
    numSpaces = alignSize - len;
    CopySpaces(s, numSpaces);
  }
  strcpy(s + numSpaces, temp);
}


// ---------- CMessageBox ----------

static const int kMaxLen = 255;

void CMessageBox::Init(const AString &title, int width)
{
  _title = title;
  _width = MyMin(width, kMaxLen);
}

void CMessageBox::ShowMessages(const char *strings[], int numStrings)
{
  const int kNumStaticStrings = 1;
  const int kNumStringsMax = 10;

  if (numStrings > kNumStringsMax)
    numStrings = kNumStringsMax;

  const char *msgItems[kNumStaticStrings + kNumStringsMax];
  msgItems[0] = _title;

  char formattedMessages[kNumStringsMax][kMaxLen + 1];

  for (int i = 0; i < numStrings; i++)
  {
    char *formattedMessage = formattedMessages[i];
    const char *s = strings[i];
    int len = (int)strlen(s);
    if (len < kMaxLen)
    {
      int size = MyMax(_width, len);
      int startPos = (size - len) / 2;
      CopySpaces(formattedMessage, startPos);
      strcpy(formattedMessage + startPos, s);
      CopySpaces(formattedMessage + startPos + len, size - startPos - len);
    }
    else
    {
      strncpy(formattedMessage, s, kMaxLen);
      formattedMessage[kMaxLen] = 0;
    }
    msgItems[kNumStaticStrings + i] = formattedMessage;
  }
  NFar::g_StartupInfo.ShowMessage(0, NULL, msgItems, kNumStaticStrings + numStrings, 0);
}


// ---------- CProgressBox ----------

void CProgressBox::Init(const AString &title, int width)
{
  CMessageBox::Init(title, width);
  _prevMessage.Empty();
  _prevPercentMessage.Empty();
  _wasShown = false;
}

void CProgressBox::Progress(const UInt64 *total, const UInt64 *completed, const AString &message)
{
  AString percentMessage;
  if (total != 0 && completed != 0)
  {
    UInt64 totalVal = *total;
    if (totalVal == 0)
      totalVal = 1;
    char buf[32];
    ConvertUInt64ToStringAligned(*completed * 100 / totalVal, buf, 3);
    strcat(buf, "%");
    percentMessage = buf;
  }
  if (message != _prevMessage || percentMessage != _prevPercentMessage || !_wasShown)
  {
    _prevMessage = message;
    _prevPercentMessage = percentMessage;
    const char *strings[] = { message, percentMessage };
    ShowMessages(strings, sizeof(strings) / sizeof(strings[0]));
    _wasShown = true;
  }
}
