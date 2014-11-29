// ExtractingFilePath.h

#ifndef __EXTRACTING_FILE_PATH_H
#define __EXTRACTING_FILE_PATH_H

#include "../../../Common/MyString.h"

UString MakePathNameFromParts(const UStringVector &parts);

/* for WIN32:
  if (isRoot == true), and  pathParts[0] contains path like "c:name",
  it thinks that "c:" is drive prefix (it's not ":name alt stream) and
  the function changes part to c_name */
void MakeCorrectPath(bool isPathFromRoot, UStringVector &pathParts, bool replaceAltStreamColon);

UString GetCorrectFsPath(const UString &path);
UString GetCorrectFullFsPath(const UString &path);

void Correct_IfEmptyLastPart(UStringVector &parts);

#endif
