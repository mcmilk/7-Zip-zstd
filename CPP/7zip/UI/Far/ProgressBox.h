// ProgressBox.h

#ifndef __PROGRESSBOX_H
#define __PROGRESSBOX_H

#include "Common/MyString.h"
#include "Common/Types.h"

class CMessageBox
{
  CSysString m_Title;
  CSysString m_Message;
  int m_NumStrings;
  int m_Width;
public:
  void Init(const CSysString &title, 
      const CSysString &message, int numStrings, int width);
  void ShowProcessMessages(const char *messages[]);
};

class CProgressBox: public CMessageBox
{
  UInt64 m_Total;
  UInt64 m_CompletedPrev;
  UInt64 m_Step;
public:
  void Init(const CSysString &title, 
      const CSysString &message, UInt64 step);
  void ShowProcessMessage(const char *message);
  void PrintPercent(UInt64 percent);
  void PrintCompeteValue(UInt64 completed);
  void SetTotal(UInt64 total);
};

#endif
