// MyXml.cpp

#include "StdAfx.h"

#include "MyXml.h"

static bool IsValidChar(char c)
{
  return
    c >= 'a' && c <= 'z' ||
    c >= 'A' && c <= 'Z' ||
    c >= '0' && c <= '9' ||
    c == '-';
}

static bool IsSpaceChar(char c)
{
  return (c == ' ' || c == '\t' || c == 0x0D || c == 0x0A);
}

#define SKIP_SPACES(s, pos) while (IsSpaceChar(s[pos])) pos++;

static bool ReadProperty(const AString &s, int &pos, CXmlProp &prop)
{
  prop.Name.Empty();
  prop.Value.Empty();
  for (; pos < s.Length(); pos++)
  {
    char c = s[pos];
    if (!IsValidChar(c))
      break;
    prop.Name += c;
  }
  
  if (prop.Name.IsEmpty())
    return false;

  SKIP_SPACES(s, pos);
  if (s[pos++] != '=')
    return false;

  SKIP_SPACES(s, pos);
  if (s[pos++] != '\"')
    return false;
  
  while (pos < s.Length())
  {
    char c = s[pos++];
    if (c == '\"')
      return true;
    prop.Value += c;
  }
  return false;
}

int CXmlItem::FindProperty(const AString &propName) const
{
  for (int i = 0; i < Props.Size(); i++)
    if (Props[i].Name == propName)
      return i;
  return -1;
}

AString CXmlItem::GetPropertyValue(const AString &propName) const
{
  int index = FindProperty(propName);
  if (index >= 0)
    return Props[index].Value;
  return AString();
}

bool CXmlItem::IsTagged(const AString &tag) const
{
  return (IsTag && Name == tag);
}

int CXmlItem::FindSubTag(const AString &tag) const
{
  for (int i = 0; i < SubItems.Size(); i++)
    if (SubItems[i].IsTagged(tag))
      return i;
  return -1;
}

AString CXmlItem::GetSubString() const
{
  if (SubItems.Size() == 1)
  {
    const CXmlItem &item = SubItems[0];
    if (!item.IsTag)
      return item.Name;
  }
  return AString();
}

AString CXmlItem::GetSubStringForTag(const AString &tag) const
{
  int index = FindSubTag(tag);
  if (index >= 0)
    return SubItems[index].GetSubString();
  return AString();
}

bool CXmlItem::ParseItems(const AString &s, int &pos, int numAllowedLevels)
{
  if (numAllowedLevels == 0)
    return false;
  SubItems.Clear();
  AString finishString = "</";
  for (;;)
  {
    SKIP_SPACES(s, pos);

    if (s.Mid(pos, finishString.Length()) == finishString)
      return true;
      
    CXmlItem item;
    if (!item.ParseItem(s, pos, numAllowedLevels - 1))
      return false;
    SubItems.Add(item);
  }
}

bool CXmlItem::ParseItem(const AString &s, int &pos, int numAllowedLevels)
{
  SKIP_SPACES(s, pos);

  int pos2 = s.Find('<', pos);
  if (pos2 < 0)
    return false;
  if (pos2 != pos)
  {
    IsTag = false;
    Name += s.Mid(pos, pos2 - pos);
    pos = pos2;
    return true;
  }
  IsTag = true;

  pos++;
  SKIP_SPACES(s, pos);

  for (; pos < s.Length(); pos++)
  {
    char c = s[pos];
    if (!IsValidChar(c))
      break;
    Name += c;
  }
  if (Name.IsEmpty() || pos == s.Length())
    return false;

  int posTemp = pos;
  for (;;)
  {
    SKIP_SPACES(s, pos);
    if (s[pos] == '/')
    {
      pos++;
      // SKIP_SPACES(s, pos);
      return (s[pos++] == '>');
    }
    if (s[pos] == '>')
    {
      if (!ParseItems(s, ++pos, numAllowedLevels))
        return false;
      AString finishString = AString("</") + Name + AString(">");
      if (s.Mid(pos, finishString.Length()) != finishString)
        return false;
      pos += finishString.Length();
      return true;
    }
    if (posTemp == pos)
      return false;

    CXmlProp prop;
    if (!ReadProperty(s, pos, prop))
      return false;
    Props.Add(prop);
    posTemp = pos;
  }
}

static bool SkipHeader(const AString &s, int &pos, const AString &startString, const AString &endString)
{
  SKIP_SPACES(s, pos);
  if (s.Mid(pos, startString.Length()) == startString)
  {
    pos = s.Find(endString, pos);
    if (pos < 0)
      return false;
    pos += endString.Length();
    SKIP_SPACES(s, pos);
  }
  return true;
}

bool CXml::Parse(const AString &s)
{
  int pos = 0;
  if (!SkipHeader(s, pos, "<?xml", "?>"))
    return false;
  if (!SkipHeader(s, pos, "<!DOCTYPE", ">"))
    return false;
  if (!Root.ParseItem(s, pos, 1000))
    return false;
  SKIP_SPACES(s, pos);
  return (pos == s.Length() && Root.IsTag);
}
