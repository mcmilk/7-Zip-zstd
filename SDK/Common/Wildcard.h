// Common/Wildcard.h

#pragma once

#ifndef __COMMON_WILDCARD_H
#define __COMMON_WILDCARD_H

#include "Common/String.h"

void SplitPathToParts(const UString &path, UStringVector &aPathParts);
// void SplitPathToParts(const AString &path, AStringVector &aPathParts);
UString ExtractFileNameFromPath(const UString &pathName);
bool DoesNameContainWildCard(const UString &pathName);
bool CompareWildCardWithName(const UString &mask, const UString &name);

namespace NWildcard {

class CCensorNode
{
  CCensorNode *_parent;
  UStringVector _names[2][2][2];
  bool CheckNameRecursive(const UString &name, bool allowed) const;
  bool CheckNameFull(const UString &name, bool allowed) const;
public:
  CObjectVector<CCensorNode> _subNodes;
  UString _name;
  CCensorNode(CCensorNode *parent, const UString &name):
      _parent(parent), _name(name) {};
  CCensorNode *FindSubNode(const UString &name);
  CCensorNode *AddSubNode(const UString &name);
  void AddItem(const UString &name, bool allowed, bool recursed, bool wildCard);
  bool CheckName(const UString &name, bool allowed, bool recursed) const;
  bool CheckNameRecursive(const UString &name) const;
  bool CheckNameFull(const UString &name) const;

  const UStringVector&GetNamesVector(bool allowed, bool recursed, bool wildCard) const;
  const UStringVector&GetAllowedNamesVector(bool recursed, bool wildCard) const
    {   return GetNamesVector(true, recursed, wildCard); } 
  const UStringVector&GetRecursedNamesVector(bool allowed, bool wildCard) const
    {   return GetNamesVector(allowed, true, wildCard); } 
  const UStringVector&GetAllowedRecursedNamesVector(bool wildCard) const
    {   return GetRecursedNamesVector(true, wildCard); } 

};

class CCensor
{
public:
  CCensorNode _head;
  CCensor(): _head(NULL, L"") {}
  void AddItem(const UString &path, bool allowed, bool recursed, bool wildCard);
  bool CheckName(const UString &path) const;
};

}

// return true if names differs only with '\' or '/' characters
bool AreTheFileNamesDirDelimiterEqual(const UString &name1, const UString &name2);

#endif
