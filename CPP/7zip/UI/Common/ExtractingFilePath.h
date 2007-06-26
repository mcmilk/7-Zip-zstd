// ExtractingFilePath.h

#ifndef __EXTRACTINGFILEPATH_H
#define __EXTRACTINGFILEPATH_H

#include "Common/MyString.h"

UString GetCorrectFileName(const UString &path);
UString GetCorrectPath(const UString &path);
void MakeCorrectPath(UStringVector &pathParts);

#endif
