// MyXml.h

#ifndef __MYXML_H
#define __MYXML_H

#include "MyString.h"

struct CXmlProp
{
  AString Name;
  AString Value;
};

class CXmlItem
{
  bool ParseItems(const AString &s, int &pos, int numAllowedLevels);

public:
  AString Name;
  bool IsTag;
  CObjectVector<CXmlProp> Props;
  CObjectVector<CXmlItem> SubItems;

  bool ParseItem(const AString &s, int &pos, int numAllowedLevels);
  
  bool IsTagged(const AString &tag) const;
  int FindProperty(const AString &propName) const;
  AString GetPropertyValue(const AString &propName) const;
  AString GetSubString() const;
  int FindSubTag(const AString &tag) const;
  AString GetSubStringForTag(const AString &tag) const;
};

struct CXml
{
  CXmlItem Root;
  bool Parse(const AString &s);
};

#endif
