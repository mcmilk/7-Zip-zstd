// ProgressBox.h

#pragma once

#ifndef __PROGRESSBOX_H
#define __PROGRESSBOX_H

#include "Common/String.h"

class CMessageBox
{
  CSysString m_Title;
  CSysString m_Message;
  int m_NumStrings;
  int m_Width;
public:
  void Init(const CSysString &aConvertingTitle, 
      const CSysString &aConvertingMessage, int aNumStrings, int aWidth);
  void ShowProcessMessages(const char *aMessages[]);
};

class CProgressBox: public CMessageBox
{
  UINT64 m_Total;
  UINT64 m_CompletedPrev;
  UINT64 m_Step;
public:
  void Init(const CSysString &aConvertingTitle, 
      const CSysString &aConvertingMessage, UINT64 aStep);
  void ShowProcessMessage(const char *aMessage);
  void PrintPercent(UINT64 aPercent);
  void PrintCompeteValue(UINT64 aCompleted);
  void SetTotal(UINT64 aTotal);
};

#endif
