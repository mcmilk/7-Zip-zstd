// Archive/LzhItem.h

#ifndef __ARCHIVE_LZH_ITEM_H
#define __ARCHIVE_LZH_ITEM_H

#include "Common/Types.h"
#include "Common/MyString.h"
#include "Common/Buffer.h"
#include "LzhHeader.h"

namespace NArchive {
namespace NLzh {

struct CExtension
{
  Byte Type;
  CByteBuffer Data;
  AString GetString() const
  {
    AString s;
    for (size_t i = 0; i < Data.GetCapacity(); i++)
    {
      char c = (char)Data[i];
      if (c == 0)
        break;
      s += c;
    }
    return s;
  }
};

struct CItem
{
public:
  AString Name;
  Byte Method[kMethodIdSize];
  UInt32 PackSize;
  UInt32 Size;
  UInt32 ModifiedTime;
  Byte Attributes;
  Byte Level;
  UInt16 CRC;
  Byte OsId;
  CObjectVector<CExtension> Extensions;

  bool IsValidMethod() const  { return (Method[0] == '-' && Method[1] == 'l' && Method[4] == '-'); }
  bool IsLhMethod() const  {return (IsValidMethod() && Method[2] == 'h'); }
  bool IsDirectory() const {return (IsLhMethod() && Method[3] == 'd'); }

  bool IsCopyMethod() const 
  {
    return (IsLhMethod() && Method[3] == '0') || 
      (IsValidMethod() && Method[2] == 'z' && Method[3] == '4');
  }
  
  bool IsLh1GroupMethod() const 
  {
    if (!IsLhMethod())
      return false;
    switch(Method[3])
    {
      case '1':
        return true;
    }
    return false;
  }
  
  bool IsLh4GroupMethod() const 
  {
    if (!IsLhMethod())
      return false;
    switch(Method[3])
    {
      case '4':
      case '5':
      case '6':
      case '7':
        return true;
    }
    return false;
  }
  
  int GetNumDictBits() const 
  {
    if (!IsLhMethod())
      return 0;
    switch(Method[3])
    {
      case '1':
        return 12;
      case '2':
        return 13;
      case '3':
        return 13;
      case '4':
        return 12;
      case '5':
        return 13;
      case '6':
        return 15;
      case '7':
        return 16;
    }
    return 0;
  }

  int FindExt(Byte type) const
  {
    for (int i = 0; i < Extensions.Size(); i++)
      if (Extensions[i].Type == type)
        return i;
    return -1;
  }
  bool GetUnixTime(UInt32 &value) const
  {
    int index = FindExt(kExtIdUnixTime);
    if (index < 0)
    {
      if (Level == 2)
      {
        value = ModifiedTime;
        return true;
      }
      return false;
    }
    const Byte *data = (const Byte *)(Extensions[index].Data);
    value = data[0] | 
        ((UInt32)data[1] << 8) | 
        ((UInt32)data[2] << 16) | 
        ((UInt32)data[3] << 24);
    return true;
  }

  AString GetDirName() const
  {
    int index = FindExt(kExtIdDirName);
    if (index < 0)
      return AString();
    return Extensions[index].GetString();
  }

  AString GetFileName() const
  {
    int index = FindExt(kExtIdFileName);
    if (index < 0)
      return Name;
    return Extensions[index].GetString();
  }

  AString GetName() const
  {
    AString dirName = GetDirName();
    dirName.Replace((char)(unsigned char)0xFF, '\\');
    if (!dirName.IsEmpty())
    {
      char c = dirName[dirName.Length() - 1];
      if (c != '\\')
        dirName += '\\';
    }
    return dirName + GetFileName();
  }
};

class CItemEx: public CItem
{
public:
  UInt64 DataPosition;
};

}}

#endif
