// Common/Wildcard.cpp

#include "StdAfx.h"

#include "WildCard.h"

static const wchar_t kPeriodChar = L'.';
static const wchar_t kAnyCharsChar = L'*';
static const wchar_t kAnyCharChar = L'?';

static const wchar_t kDirDelimiter1 = L'\\';
static const wchar_t kDirDelimiter2 = L'/';

static const wchar_t kDiskNameDelimiterChar = ':';

static const UString kRootDirName = L"";

static const wchar_t kSpaceChar = L' ';

static const UString kWildCardCharSet = L"?*";

static const UString kIllegalWildCardFileNameChars=
  L"\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\xD\xE\xF"
  L"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
  L"\"/:<>\\|";

static const UString kIllegalFileNameChars = kIllegalWildCardFileNameChars + 
    kWildCardCharSet;

static inline bool IsCharDirLimiter(wchar_t c)
{
  return (c == kDirDelimiter1 || c == kDirDelimiter2);
}

// -----------------------------------------
// this function tests is name matches mask
// ? - any wchar_t or empty
// * - any characters or empty

static bool EnhancedMaskTest(const UString &mask, int maskPos, 
    const UString &name, int namePos)
{
  int maskLen = mask.Length() - maskPos;
  int nameLen = name.Length() - namePos;
  if (maskLen == 0) 
    if (nameLen == 0)
      return true;
    else
      return false;
  wchar_t maskChar = mask[maskPos];
  if(maskChar == kAnyCharChar)
  {
    if (EnhancedMaskTest(mask, maskPos + 1, name, namePos))
      return true;
    if (nameLen == 0) 
      return false;
    return EnhancedMaskTest(mask,  maskPos + 1, name, namePos + 1);
  }
  else if(maskChar == kAnyCharsChar)
  {
    if (EnhancedMaskTest(mask, maskPos + 1, name, namePos))
      return true;
    if (nameLen == 0) 
      return false;
    return EnhancedMaskTest(mask, maskPos, name, namePos + 1);
  }
  else
  {
    if (toupper(maskChar) != toupper(name[namePos]))
      return false;
    return EnhancedMaskTest(mask,  maskPos + 1, name, namePos + 1);
  }
}

// --------------------------------------------------
// Splits path to strings

void SplitPathToParts(const UString &path, UStringVector &pathParts)
{
  pathParts.Clear();
  UString name;
  int len = path.Length();
  if (len == 0)
    return;
  for (int i = 0; i < len; i++)
  {
    wchar_t c = path[i];
    if (IsCharDirLimiter(c))
    {
      pathParts.Add(name);
      name.Empty();
    }
    else
      name += c;
  }
  pathParts.Add(name);
}

/*
void SplitPathToParts(const AString &path, AStringVector &pathParts)
{
  pathParts.Clear();
  AString name;
  int len = path.Length();
  if (len == 0)
    return;
  for (int i = 0; i < len; i++)
  {
    wchar_t c = path[i];
    if (IsCharDirLimiter(c))
    {
      pathParts.Add(name);
      name.Empty();
    }
    else
      name += path[i];
  }
  pathParts.Add(name);
}
*/

// --------------------------------------------------
// ExtractFileNameFromPath

UString ExtractFileNameFromPath(const UString &pathName)
{
  UString result;
  int len = pathName.Length();
  for(int i = len - 1; i >= 0; i--)
    if(IsCharDirLimiter(pathName[i]))
      return pathName.Mid(i + 1);
  return pathName;
}

bool CompareWildCardWithName(const UString &mask, const UString &name)
{
  return EnhancedMaskTest(mask, 0, name, 0);
}

bool DoesNameContainWildCard(const UString &pathName)
{
  return (pathName.FindOneOf(kWildCardCharSet) >= 0);
}


// ----------------------------------------------------------'
// NWildcard

namespace NWildcard {

static inline int BoolToIndex(bool value)
{
  return value ? 1: 0;
}

const UStringVector& CCensorNode::GetNamesVector(bool allowed, bool recursed, bool wildCard) const
{
  return _names[BoolToIndex(allowed)][BoolToIndex(recursed)][BoolToIndex(wildCard)];
}

void CCensorNode::AddItem(const UString &name, bool allowed, bool recursed, bool wildCard)
{
  _names[BoolToIndex(allowed)][BoolToIndex(recursed)][BoolToIndex(wildCard)].Add(name);
}

CCensorNode *CCensorNode::FindSubNode(const UString &name)
{
  for (int i = 0; i < SubNodes.Size(); i++)
    if (name.CollateNoCase(SubNodes[i].Name) == 0)
      return &SubNodes[i];
  return NULL;
}

CCensorNode *CCensorNode::AddSubNode(const UString &name)
{
  CCensorNode *subNode = FindSubNode(name);
  if (subNode != NULL)
    return subNode;
  SubNodes.Add(CCensorNode(this, name));
  return &SubNodes.Back();
}

int FindString(const UStringVector &strings, const UString &string)
{
  for (int i = 0; i < strings.Size(); i++)
    if (string.CollateNoCase(strings[i]) == 0)
      return i;
  return -1;
}

int FindInWildcardVector(const UStringVector &strings, const UString &string)
{
  for (int i = 0; i < strings.Size(); i++)
    if (CompareWildCardWithName(strings[i], string))
      return i;
  return -1;
}

bool CCensorNode::CheckName(const UString &name, bool allowed, bool recursed) const
{
  if (FindString(_names[BoolToIndex(allowed)][BoolToIndex(recursed)][0], name) >= 0)
    return true;
  if (FindInWildcardVector(_names[BoolToIndex(allowed)][BoolToIndex(recursed)][1], name) >= 0)
    return true;
  return false;
}

bool CCensorNode::CheckNameRecursive(const UString &name, bool allowed) const
{
  const CCensorNode *curItem = this;
  while (curItem != NULL)
  {
    if (curItem->CheckName(name, allowed, true))
      return true;
    curItem = curItem->_parent;
  }
  return false;
}

bool CCensorNode::CheckNameRecursive(const UString &name) const
{
  if (!CheckNameRecursive(name, true))
    return false;
   return !CheckNameRecursive(name, false);
}

bool CCensorNode::CheckNameFull(const UString &name, bool allowed) const
{
  if (CheckName(name, allowed, false))
    return true;
  return CheckNameRecursive(name, allowed);
}

bool CCensorNode::CheckNameFull(const UString &name) const
{
  if (!CheckNameFull(name, true))
    return false;
  return !CheckNameFull(name, false);
}

////////////////////////////////////
// CCensor

void CCensor::AddItem(const UString &path, bool allowed, bool recursed, bool wildCard)
{
  UStringVector pathParts;
  SplitPathToParts(path, pathParts);
  int numParts = pathParts.Size();
  if (numParts == 0)
    throw "Empty path";
  CCensorNode *curItem = &_head;
  for(int i = 0; i < numParts - 1; i++)
    curItem = curItem->AddSubNode(pathParts[i]);
  curItem->AddItem(pathParts[i], allowed, recursed, wildCard);
}


bool CCensor::CheckName(const UString &path) const
{
  UStringVector pathParts;
  SplitPathToParts(path, pathParts);
  int numParts = pathParts.Size();
  if (numParts == 0)
    throw "Empty path";
  const CCensorNode *curItem = &_head;
  const UString &name = pathParts[numParts - 1];
  for(int i = 0; i < numParts - 1; i++)
  {
    const CCensorNode *nextItem = ((CCensorNode *)curItem)->FindSubNode(pathParts[i]);
    if (nextItem == NULL)
      return curItem->CheckNameRecursive(name);
    curItem = nextItem;
  }
  return curItem->CheckNameFull(name);
}

}

////////////////////////////////////
// Filename and WildCard function 

static bool TestStringLengthAndBounds(const UString &string)
{
  if (string.Length() <= 0)
    return false;
  return (string.ReverseFind(kSpaceChar) != string.Length() - 1);
}
bool IsFileNameLegal(const UString &name)
{
  if (!TestStringLengthAndBounds(name))
    return false;
  return (name.FindOneOf(kIllegalFileNameChars) < 0);
}

bool IsWildCardFileNameLegal(const UString &name)
{
  if (!TestStringLengthAndBounds(name))
    return false;
  return (name.FindOneOf(kIllegalWildCardFileNameChars) < 0);
}

bool IsFilePathLegal(const UString &path)
{
  UStringVector pathParts; 
  SplitPathToParts(path, pathParts);
  int count = pathParts.Size();
  if (count == 0)
    return false;
  for(int i = 0; i < count; i++)
    if (!IsFileNameLegal(pathParts[i]))
      return false;
  return true;
}

bool IsWildCardFilePathLegal(const UString &path)
{
  UStringVector pathParts; 
  SplitPathToParts(path, pathParts);
  int count = pathParts.Size();
  if (count == 0)
    return false;
  for(int i = 0; i < count - 1; i++)
    if (!IsFileNameLegal(pathParts[i]))
      return false;
  return IsWildCardFileNameLegal(pathParts[count - 1]);
}

static bool IsCharAPrefixDelimiter(wchar_t c)
{
  return (IsCharDirLimiter(c) || c == kDiskNameDelimiterChar);
}

bool AreTheFileNamesDirDelimiterEqual(const UString &name1, const UString &name2)
{
  if(name1.Length() != name2.Length())
    return false;
  for(int i = 0; i < name1.Length(); i++)
  {
    wchar_t char1 = name1[i], char2 = name2[i];
    if (char1 == char2)
      continue;
    if (IsCharDirLimiter(char1) && IsCharDirLimiter(char2))
      continue;
    return false;
  }
  return true;
}

