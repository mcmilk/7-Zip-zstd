// Archive/cpio/ItemInfo.h

#ifndef __ARCHIVE_CPIO_ITEMINFO_H
#define __ARCHIVE_CPIO_ITEMINFO_H

#include <sys/stat.h>

#include "Common/Types.h"
#include "Common/MyString.h"
#include "CpioHeader.h"

namespace NArchive {
namespace NCpio {

struct CItem
{
  AString Name;
  UInt32 inode;
  UInt32 Mode;
  UInt32 UID;
  UInt32 GID;
  UInt32 Size;
  UInt32 ModificationTime;

  // char LinkFlag;
  // AString LinkName; ?????
  char Magic[8];
  UInt32 NumLinks;
  UInt32 DevMajor;
  UInt32 DevMinor;
  UInt32 RDevMajor;
  UInt32 RDevMinor;
  UInt32 ChkSum;

  UInt32 Align;

  bool IsDirectory() const 
#ifdef _WIN32
    { return (Mode & _S_IFMT) == _S_IFDIR; }
#else
    { return (Mode & S_IFMT) == S_IFDIR; }
#endif
};

class CItemEx: public CItem
{
public:
  UInt64 HeaderPosition;
  UInt32 HeaderSize;
  UInt64 GetDataPosition() const { return HeaderPosition + HeaderSize; };
};

}}

#endif
