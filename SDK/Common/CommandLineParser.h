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
  CSysStringVector PostStrings;
  int PostCharIndex;
  CSwitchResult(): ThereIs(false) {};
};
  
class CParser
{
  int m_NumSwitches;
  CSwitchResult *m_Switches;
  bool ParseString(const CSysString &aString, const CSwitchForm *aSwitchForms); 
public:
  CSysStringVector m_NonSwitchStrings;
  CParser(int aNumSwitches);
  ~CParser();
  void ParseStrings(const CSwitchForm *aSwitchForms, 
    const CSysStringVector &aCommandStrings);
  const CSwitchResult& operator[](size_t anIndex) const;
};

/////////////////////////////////
// Command parsing procedures

struct CCommandForm
{
  char *IDString;
  bool PostStringMode;
};

// Returns: Index of form and aPostString; -1, if there is no match
int ParseCommand(int aNumCommandForms, const CCommandForm *aCommandForms, 
    const CSysString &aCommandString, CSysString &aPostString);

struct CCommandSubCharsSet
{
  char *Chars;
  bool EmptyAllowed;
};

// Returns: anIndexes of finded chars; -1 if there is no match
bool ParseSubCharsCommand(int aNumForms, const CCommandSubCharsSet *aForms, 
    const CSysString &aCommandString, CIntVector &anIndexes);

}

#endif
