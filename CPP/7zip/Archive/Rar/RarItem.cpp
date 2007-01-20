// RarItem.cpp

#include "StdAfx.h"

#include "RarItem.h"
#include "RarHeader.h"

namespace NArchive{
namespace NRar{

bool CItem::IsEncrypted() const
  { return (Flags & NHeader::NFile::kEncrypted) != 0; }
bool CItem::IsSolid() const
  { return (Flags & NHeader::NFile::kSolid) != 0; }
bool CItem::IsCommented() const
  { return (Flags & NHeader::NFile::kComment) != 0; }
bool CItem::IsSplitBefore() const
  { return (Flags & NHeader::NFile::kSplitBefore) != 0; }
bool CItem::IsSplitAfter() const
  { return (Flags & NHeader::NFile::kSplitAfter) != 0; }
bool CItem::HasSalt() const
  { return (Flags & NHeader::NFile::kSalt) != 0; }
bool CItem::HasExtTime() const
  { return (Flags & NHeader::NFile::kExtTime) != 0; }
bool CItem::HasUnicodeName() const
  { return (Flags & NHeader::NFile::kUnicodeName) != 0; }
bool CItem::IsOldVersion() const
  { return (Flags & NHeader::NFile::kOldVersion) != 0; }

bool CItem::IgnoreItem() const
{ 
  switch(HostOS)
  {
  case NHeader::NFile::kHostMSDOS:
  case NHeader::NFile::kHostOS2:
  case NHeader::NFile::kHostWin32:
    return ((Attributes & NHeader::NFile::kLabelFileAttribute) != 0); 
  }
  return false;
}

UInt32  CItem::GetDictSize() const
{ return (Flags >> NHeader::NFile::kDictBitStart) & NHeader::NFile::kDictMask; }

bool CItem::IsDirectory() const
{ 
  if (GetDictSize() == NHeader::NFile::kDictDirectoryValue)
    return true;
  switch(HostOS)
  {
    case NHeader::NFile::kHostMSDOS:
    case NHeader::NFile::kHostOS2:
    case NHeader::NFile::kHostWin32:
      if ((Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        return true;
  }
  return false;
}

UInt32 CItem::GetWinAttributes() const
{
  UInt32 winAttributes;
  switch(HostOS)
  {
  case NHeader::NFile::kHostMSDOS:
  case NHeader::NFile::kHostOS2:
  case NHeader::NFile::kHostWin32:
    winAttributes = Attributes; 
    break;
  default:
    winAttributes = 0; // must be converted from unix value;; 
  }
  if (IsDirectory())       // test it;
    winAttributes |= NHeader::NFile::kWinFileDirectoryAttributeMask;
  return winAttributes;
}

void CItem::ClearFlags()
{ Flags = 0; }

void CItem::SetFlagBits(int startBitNumber, int numBits, int value)
{  
  UInt16 mask = (UInt16)(((1 << numBits) - 1) << startBitNumber);
  Flags &= ~mask;
  Flags |= value << startBitNumber;
}

void CItem::SetBitMask(int bitMask, bool enable)
{  
  if(enable) 
    Flags |= bitMask;
  else
    Flags &= ~bitMask;
}

void CItem::SetDictSize(UInt32 size)
{ 
  SetFlagBits(NHeader::NFile::kDictBitStart, NHeader::NFile::kNumDictBits, (size & NHeader::NFile::kDictMask));
}

void CItem::SetAsDirectory(bool directory)
{
  if (directory)
    SetDictSize(NHeader::NFile::kDictDirectoryValue);
}

void CItem::SetEncrypted(bool encrypted)
  { SetBitMask(NHeader::NFile::kEncrypted, encrypted); }
void CItem::SetSolid(bool solid)
  { SetBitMask(NHeader::NFile::kSolid, solid); }
void CItem::SetCommented(bool commented)
  { SetBitMask(NHeader::NFile::kComment, commented); }
void CItem::SetSplitBefore(bool splitBefore)
  { SetBitMask(NHeader::NFile::kSplitBefore, splitBefore); }
void CItem::SetSplitAfter(bool splitAfter)
  { SetBitMask(NHeader::NFile::kSplitAfter, splitAfter); }

}}
