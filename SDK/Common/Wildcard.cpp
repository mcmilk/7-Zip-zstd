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

static inline bool IsCharDirLimiter(wchar_t aChar)
{
  return (aChar == kDirDelimiter1 || aChar == kDirDelimiter2);
}

// -----------------------------------------
// this function tests is aName matches aMask
// ? - any wchar_t or empty
// * - any characters or empty

static bool EnhancedMaskTest(const UString &aMask, int aMaskPos, 
    const UString &aName, int aNamePos)
{
  int aLenMask = aMask.Length() - aMaskPos;
  int aLenName = aName.Length() - aNamePos;
  if (aLenMask == 0) 
    if (aLenName == 0)
      return true;
    else
      return false;
  wchar_t aMaskChar = aMask[aMaskPos];
  if(aMaskChar == kAnyCharChar)
  {
    if (EnhancedMaskTest(aMask, aMaskPos + 1, aName, aNamePos))
      return true;
    if (aLenName == 0) 
      return false;
    return EnhancedMaskTest(aMask,  aMaskPos + 1, aName, aNamePos + 1);
  }
  else if(aMaskChar == kAnyCharsChar)
  {
    if (EnhancedMaskTest(aMask, aMaskPos + 1, aName, aNamePos))
      return true;
    if (aLenName == 0) 
      return false;
    return EnhancedMaskTest(aMask, aMaskPos, aName, aNamePos + 1);
  }
  else
  {
    if (toupper(aMaskChar) != toupper(aName[aNamePos]))
      return false;
    return EnhancedMaskTest(aMask,  aMaskPos + 1, aName, aNamePos + 1);
  }
}

// --------------------------------------------------
// Splits aPath to strings

void SplitPathToParts(const UString &aPath, UStringVector &aPathParts)
{
  aPathParts.Clear();
  UString aName;
  int aLen = aPath.Length();
  if (aLen == 0)
    return;
  for (int i = 0; i < aLen; i++)
  {
    wchar_t c = aPath[i];
    if (IsCharDirLimiter(c))
    {
      aPathParts.Add(aName);
      aName.Empty();
    }
    else
      aName += c;
  }
  aPathParts.Add(aName);
}

/*
void SplitPathToParts(const AString &aPath, AStringVector &aPathParts)
{
  aPathParts.Clear();
  AString aName;
  int aLen = aPath.Length();
  if (aLen == 0)
    return;
  for (int i = 0; i < aLen; i++)
  {
    wchar_t c = aPath[i];
    if (IsCharDirLimiter(c))
    {
      aPathParts.Add(aName);
      aName.Empty();
    }
    else
      aName += aPath[i];
  }
  aPathParts.Add(aName);
}
*/

// --------------------------------------------------
// ExtractFileNameFromPath

UString ExtractFileNameFromPath(const UString &aPathName)
{
  UString aResult;
  int aLen = aPathName.Length();
  for(int i = aLen - 1; i >= 0; i--)
    if(IsCharDirLimiter(aPathName[i]))
      return aPathName.Mid(i + 1);
  return aPathName;
}

bool CompareWildCardWithName(const UString &aMask, const UString &aName)
{
  return EnhancedMaskTest(aMask, 0, aName, 0);
}

bool DoesNameContainWildCard(const UString &aPathName)
{
  return (aPathName.FindOneOf(kWildCardCharSet) >= 0);
}


// ----------------------------------------------------------'
// NWildcard

namespace NWildcard {

static inline BoolToIndex(bool aValue)
{
  return aValue ? 1: 0;
}

const UStringVector& CCensorNode::GetNamesVector(bool anAllowed, bool aRecursed, bool aWildCard) const
{
  return m_Names[BoolToIndex(anAllowed)][BoolToIndex(aRecursed)][BoolToIndex(aWildCard)];
}

void CCensorNode::AddItem(const UString &aName, bool anAllowed, bool aRecursed, bool aWildCard)
{
  m_Names[BoolToIndex(anAllowed)][BoolToIndex(aRecursed)][BoolToIndex(aWildCard)].Add(aName);
}

CCensorNode *CCensorNode::FindSubNode(const UString &aName)
{
  for (int i = 0; i < m_SubNodes.Size(); i++)
    if (aName.CollateNoCase(m_SubNodes[i].m_Name) == 0)
      return &m_SubNodes[i];
  return NULL;
}

CCensorNode *CCensorNode::AddSubNode(const UString &aName)
{
  CCensorNode *aSubNode = FindSubNode(aName);
  if (aSubNode != NULL)
    return aSubNode;
  m_SubNodes.Add(CCensorNode(this, aName));
  return &m_SubNodes.Back();
}

int FindString(const UStringVector &aStrings, const UString &aString)
{
  for (int i = 0; i < aStrings.Size(); i++)
    if (aString.CollateNoCase(aStrings[i]) == 0)
      return i;
  return -1;
}

int FindInWildcardVector(const UStringVector &aStrings, const UString &aString)
{
  for (int i = 0; i < aStrings.Size(); i++)
    if (CompareWildCardWithName(aStrings[i], aString))
      return i;
  return -1;
}

bool CCensorNode::CheckName(const UString &aName, bool anAllowed, bool aRecursed) const
{
  if (FindString(m_Names[BoolToIndex(anAllowed)][BoolToIndex(aRecursed)][0], aName) >= 0)
    return true;
  if (FindInWildcardVector(m_Names[BoolToIndex(anAllowed)][BoolToIndex(aRecursed)][1], aName) >= 0)
    return true;
  return false;
}

bool CCensorNode::CheckNameRecursive(const UString &aName, bool anAllowed) const
{
  const CCensorNode *aCurItem = this;
  while (aCurItem != NULL)
  {
    if (aCurItem->CheckName(aName, anAllowed, true))
      return true;
    aCurItem = aCurItem->m_Parent;
  }
  return false;
}

bool CCensorNode::CheckNameRecursive(const UString &aName) const
{
  if (!CheckNameRecursive(aName, true))
    return false;
   return !CheckNameRecursive(aName, false);
}

bool CCensorNode::CheckNameFull(const UString &aName, bool anAllowed) const
{
  if (CheckName(aName, anAllowed, false))
    return true;
  return CheckNameRecursive(aName, anAllowed);
}

bool CCensorNode::CheckNameFull(const UString &aName) const
{
  if (!CheckNameFull(aName, true))
    return false;
  return !CheckNameFull(aName, false);
}

////////////////////////////////////
// CCensor

void CCensor::AddItem(const UString &aPath, bool anAllowed, bool aRecursed, bool aWildCard)
{
  UStringVector aPathParts;
  SplitPathToParts(aPath, aPathParts);
  int aNumParts = aPathParts.Size();
  if (aNumParts == 0)
    throw "Empty path";
  CCensorNode *aCurItem = &m_Head;
  for(int i = 0; i < aNumParts - 1; i++)
    aCurItem = aCurItem->AddSubNode(aPathParts[i]);
  aCurItem->AddItem(aPathParts[i], anAllowed, aRecursed, aWildCard);
}


bool CCensor::CheckName(const UString &aPath) const
{
  UStringVector aPathParts;
  SplitPathToParts(aPath, aPathParts);
  int aNumParts = aPathParts.Size();
  if (aNumParts == 0)
    throw "Empty path";
  const CCensorNode *aCurItem = &m_Head;
  const UString &aName = aPathParts[aNumParts - 1];
  for(int i = 0; i < aNumParts - 1; i++)
  {
    const CCensorNode *aNextItem = ((CCensorNode *)aCurItem)->FindSubNode(aPathParts[i]);
    if (aNextItem == NULL)
      return aCurItem->CheckNameRecursive(aName);
    aCurItem = aNextItem;
  }
  return aCurItem->CheckNameFull(aName);
}

}

////////////////////////////////////
// Filename and WildCard function 

static bool TestStringLengthAndBounds(const UString &aString)
{
  if (aString.Length() <= 0)
    return false;
  return (aString.ReverseFind(kSpaceChar) != aString.Length() - 1);
}
bool IsFileNameLegal(const UString &aName)
{
  if (!TestStringLengthAndBounds(aName))
    return false;
  return (aName.FindOneOf(kIllegalFileNameChars) < 0);
}

bool IsWildCardFileNameLegal(const UString &aName)
{
  if (!TestStringLengthAndBounds(aName))
    return false;
  return (aName.FindOneOf(kIllegalWildCardFileNameChars) < 0);
}

bool IsFilePathLegal(const UString &aPath)
{
  UStringVector aPathParts; 
  SplitPathToParts(aPath, aPathParts);
  int aCount = aPathParts.Size();
  if (aCount == 0)
    return false;
  for(int i = 0; i < aCount; i++)
    if (!IsFileNameLegal(aPathParts[i]))
      return false;
  return true;
}

bool IsWildCardFilePathLegal(const UString &aPath)
{
  UStringVector aPathParts; 
  SplitPathToParts(aPath, aPathParts);
  int aCount = aPathParts.Size();
  if (aCount == 0)
    return false;
  for(int i = 0; i < aCount - 1; i++)
    if (!IsFileNameLegal(aPathParts[i]))
      return false;
  return IsWildCardFileNameLegal(aPathParts[aCount - 1]);
}

static bool IsCharAPrefixDelimiter(wchar_t aChar)
{
  return (IsCharDirLimiter(aChar) || aChar == kDiskNameDelimiterChar);
}

bool AreTheFileNamesDirDelimiterEqual(const UString &aName1, const UString &aName2)
{
  if(aName1.Length() != aName2.Length())
    return false;
  for(int i = 0; i < aName1.Length(); i++)
  {
    wchar_t aChar1 = aName1[i], aChar2 = aName2[i];
    if (aChar1 == aChar2)
      continue;
    if (IsCharDirLimiter(aChar1) && IsCharDirLimiter(aChar2))
      continue;
    return false;
  }
  return true;
}

