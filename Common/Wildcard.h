// Common/Wildcard.h

#ifndef __COMMON_WILDCARD_H
#define __COMMON_WILDCARD_H

#include "Common/String.h"

void SplitPathToParts(const UString &path, UStringVector &pathParts);
void SplitPathToParts(const UString &path, UString &dirPrefix, UString &name);
UString ExtractDirPrefixFromPath(const UString &path);
UString ExtractFileNameFromPath(const UString &path);
bool DoesNameContainWildCard(const UString &path);
bool CompareWildCardWithName(const UString &mask, const UString &name);

namespace NWildcard {

struct CItem
{
  UStringVector PathParts;
  bool Include;
  bool Recursive;
  bool ForFile;
  bool ForDir;
  bool CheckPath(const UStringVector &pathParts, bool isFile) const;
};

class CCensorNode
{
  UString Name;
  CCensorNode *Parent;
  bool CheckPathCurrent(const UStringVector &pathParts, bool isFile, bool &include) const;
public:
  CCensorNode(): Parent(0) { };
  CCensorNode(const UString &name, CCensorNode *parent): 
      Name(name), Parent(parent) { };
  CObjectVector<CCensorNode> SubNodes;
  CObjectVector<CItem> Items;

  int FindSubNode(const UString &path) const;

  void AddItem(CItem &item);
  void AddItem(const UString &path, bool include, bool recursive, bool forFile, bool forDir);
  void AddItem2(const UString &path, bool include, bool recursive);

  bool NeedCheckSubDirs() const;

  bool CheckPath(UStringVector &pathParts, bool isFile, bool &include) const;
  bool CheckPath(const UString &path, bool isFile, bool &include) const;
  bool CheckPath(const UString &path, bool isFile) const;

  bool CheckPathToRoot(UStringVector &pathParts, bool isFile, bool &include) const;
  bool CheckPathToRoot(UStringVector &pathParts, bool isFile) const;
  bool CheckPathToRoot(const UString &path, bool isFile) const;

};

struct CPair
{
  UString Prefix;
  CCensorNode Head;
  CPair(const UString &prefix): Prefix(prefix) { };
};

class CCensor
{
  int FindPrefix(const UString &prefix) const;
public:
  CObjectVector<CPair> Pairs;
  bool AllAreRelative() const
    { return (Pairs.Size() == 1 && Pairs.Front().Prefix.IsEmpty()); }
  void AddItem(const UString &path, bool include, bool recursive);
  bool CheckPath(const UString &path, bool isFile) const;
};

}

// return true if names differs only with '\' or '/' characters
bool AreTheFileNamesDirDelimiterEqual(const UString &name1, const UString &name2);

#endif
