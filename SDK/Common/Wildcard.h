// Common/Wildcard.h

#pragma once

#ifndef __COMMON_WILDCARD_H
#define __COMMON_WILDCARD_H

#include "Common/String.h"

void SplitPathToParts(const UString &aPath, UStringVector &aPathParts);
// void SplitPathToParts(const AString &aPath, AStringVector &aPathParts);
UString ExtractFileNameFromPath(const UString &aPathName);
bool DoesNameContainWildCard(const UString &aPathName);
bool CompareWildCardWithName(const UString &aMask, const UString &aName);

namespace NWildcard {

class CCensorNode
{
  CCensorNode *m_Parent;
  UStringVector m_Names[2][2][2];
  bool CheckNameRecursive(const UString &aName, bool anAllowed) const;
  bool CheckNameFull(const UString &aName, bool anAllowed) const;
public:
  CObjectVector<CCensorNode> m_SubNodes;
  UString m_Name;
  CCensorNode(CCensorNode *aParent, const UString &aName):
      m_Parent(aParent), m_Name(aName) {};
  CCensorNode *FindSubNode(const UString &aName);
  CCensorNode *AddSubNode(const UString &aName);
  void AddItem(const UString &aName, bool anAllowed, bool aRecursed, bool aWildCard);
  bool CheckName(const UString &aName, bool anAllowed, bool aRecursed) const;
  bool CheckNameRecursive(const UString &aName) const;
  bool CheckNameFull(const UString &aName) const;

  const UStringVector&GetNamesVector(bool anAllowed, bool aRecursed, bool aWildCard) const;
  const UStringVector&GetAllowedNamesVector(bool aRecursed, bool aWildCard) const
    {   return GetNamesVector(true, aRecursed, aWildCard); } 
  const UStringVector&GetRecursedNamesVector(bool anAllowed, bool aWildCard) const
    {   return GetNamesVector(anAllowed, true, aWildCard); } 
  const UStringVector&GetAllowedRecursedNamesVector(bool aWildCard) const
    {   return GetRecursedNamesVector(true, aWildCard); } 

};

class CCensor
{
public:
  CCensorNode m_Head;
  CCensor(): m_Head(NULL, L"") {}
  void AddItem(const UString &aPath, bool anAllowed, bool aRecursed, bool aWildCard);
  bool CheckName(const UString &aPath) const;
};

}

// return true if names differs only with '\' or '/' characters
bool AreTheFileNamesDirDelimiterEqual(const UString &aName1, const UString &aName2);

#endif
