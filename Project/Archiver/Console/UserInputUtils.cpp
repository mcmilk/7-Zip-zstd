// UserInputUtils.cpp

#include "StdAfx.h"

#include "UserInputUtils.h"
#include "Common/StdInStream.h"
#include "Common/StdOutStream.h"

static const char kYes  = 'Y';
static const char kNo   = 'N';
static const char kYesAll  = 'A';
static const char kNoAll   = 'S';
static const char kQuit    = 'Q';

static const char *kFirstQuestionMessage = "? ";
static const char *kHelpQuestionMessage = 
  "Yes, No, Always (yes for all), Skip (no for all) or Quit? ";

// return true if pressed Quite;
// in: anAll
// out: anAll, anYes;

NUserAnswerMode::EEnum ScanUserYesNoAllQuit()
{
  g_StdOut << kFirstQuestionMessage;
  do
  {
    AString aScannedString = g_StdIn.ScanStringUntilNewLine();
    aScannedString.Trim();
    if(!aScannedString.IsEmpty())
      switch(toupper(aScannedString[0]))
      {
        case kYes:
          return NUserAnswerMode::kYes;
        case kNo:
          return NUserAnswerMode::kNo;
        case kYesAll:
          return NUserAnswerMode::kYesAll;
        case kNoAll:
          return NUserAnswerMode::kNoAll;
        case kQuit:
          return NUserAnswerMode::kQuit;
      }
    g_StdOut << kHelpQuestionMessage;
  }
  while(true);
}
