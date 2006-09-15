// UserInputUtils.cpp

#include "StdAfx.h"

#include "Common/StdInStream.h"
#include "Common/StringConvert.h"

#include "UserInputUtils.h"

static const char kYes = 'Y';
static const char kNo = 'N';
static const char kYesAll = 'A';
static const char kNoAll = 'S';
static const char kAutoRename = 'U';
static const char kQuit = 'Q';

static const char *kFirstQuestionMessage = "?\n";
static const char *kHelpQuestionMessage = 
  "(Y)es / (N)o / (A)lways / (S)kip all / A(u)to rename / (Q)uit? ";

// return true if pressed Quite;
// in: anAll
// out: anAll, anYes;

NUserAnswerMode::EEnum ScanUserYesNoAllQuit(CStdOutStream *outStream)
{
  (*outStream) << kFirstQuestionMessage;
  for(;;)
  {
    (*outStream) << kHelpQuestionMessage;
    AString scannedString = g_StdIn.ScanStringUntilNewLine();
    scannedString.Trim();
    if(!scannedString.IsEmpty())
      switch(::MyCharUpper(scannedString[0]))
      {
        case kYes:
          return NUserAnswerMode::kYes;
        case kNo:
          return NUserAnswerMode::kNo;
        case kYesAll:
          return NUserAnswerMode::kYesAll;
        case kNoAll:
          return NUserAnswerMode::kNoAll;
        case kAutoRename:
          return NUserAnswerMode::kAutoRename;
        case kQuit:
          return NUserAnswerMode::kQuit;
      }
  }
}

UString GetPassword(CStdOutStream *outStream)
{
  (*outStream) << "\nEnter password:";
  outStream->Flush();
  AString oemPassword = g_StdIn.ScanStringUntilNewLine();
  return MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
}
