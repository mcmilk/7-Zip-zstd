// Common/CommandLineParser.h

#pragma once

#ifndef __COMMON_COMMANDLINEPARSER_H
#define __COMMON_COMMANDLINEPARSER_H

#include "Common/String.h"

namespace NComandLineParser {

namespace NSwitchType {
  enum EEnum
  { 
    kSimple,
    kPostMinus,
    kLimitedPostString,
    kUnLimitedPostString,
    kPostChar
  };
}

struct CSwitchForm
{
  const char *IDString;
  NSwitchType::EEnum Type;
  bool Multi;
  int MinLen;
  int MaxLen;
  const char *PostCharSet;
};

struct CSwitchResult
{
  bool ThereIs;
  bool WithMinus;
  AStringVector PostStrings;
  int PostCharIndex;
  CSwitchResult(): ThereIs(false) {};
};
  
class CParser
{
  int _numSwitches;
  CSwitchResult *_switches;
  bool ParseString(const AString &string, const CSwitchForm *switchForms); 
public:
  AStringVector _nonSwitchStrings;
  CParser(int numSwitches);
  ~CParser();
  void ParseStrings(const CSwitchForm *switchForms, 
    const AStringVector &commandStrings);
  const CSwitchResult& operator[](size_t index) const;
};

/////////////////////////////////
// Command parsing procedures

struct CCommandForm
{
  char *IDString;
  bool PostStringMode;
};

// Returns: Index of form and postString; -1, if there is no match
int ParseCommand(int numCommandForms, const CCommandForm *commandForms, 
    const AString &commandString, AString &postString);

struct CCommandSubCharsSet
{
  char *Chars;
  bool EmptyAllowed;
};

// Returns: indices of finded chars; -1 if there is no match
bool ParseSubCharsCommand(int numForms, const CCommandSubCharsSet *forms, 
    const AString &commandString, CIntVector &indices);

}

#endif
