// UserInputUtils.h

#ifndef __USERINPUTUTILS_H
#define __USERINPUTUTILS_H

namespace NUserAnswerMode {

enum EEnum
{
  kYes,
  kNo,
  kYesAll,
  kNoAll,
  kAutoRename,
  kQuit,
};
}

NUserAnswerMode::EEnum ScanUserYesNoAllQuit();

#endif
