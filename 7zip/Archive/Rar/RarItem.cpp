// RarItem.cpp

#include "stdafx.h"

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
  return (GetDictSize() == NHeader::NFile::kDictDirectoryValue);
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

void CItem::SetFlagBits(int aStartBitNumber, int aNumBits, int aValue)
{  
  UInt16 mask = ((1 << aNumBits) - 1) << aStartBitNumber;
  Flags &= ~mask;
  Flags |= aValue << aStartBitNumber;
}

void CItem::SetBitMask(int aBitMask, bool anEnable)
{  
  if(anEnable) 
    Flags |= aBitMask;
  else
    Flags &= ~aBitMask;
}

void CItem::SetDictSize(UInt32 aSize)
{ 
  SetFlagBits(NHeader::NFile::kDictBitStart, NHeader::NFile::kNumDictBits, (aSize & NHeader::NFile::kDictMask));
}

void CItem::SetAsDirectory(bool aDirectory)
{
  if (aDirectory)
    SetDictSize(NHeader::NFile::kDictDirectoryValue);
}

void CItem::SetEncrypted(bool anEncrypted)
  { SetBitMask(NHeader::NFile::kEncrypted, anEncrypted); }
void CItem::SetSolid(bool aSolid)
  { SetBitMask(NHeader::NFile::kSolid, aSolid); }
void CItem::SetCommented(bool aCommented)
  { SetBitMask(NHeader::NFile::kComment, aCommented); }
void CItem::SetSplitBefore(bool aSplitBefore)
  { SetBitMask(NHeader::NFile::kSplitBefore, aSplitBefore); }
void CItem::SetSplitAfter(bool aSplitAfter)
  { SetBitMask(NHeader::NFile::kSplitAfter, aSplitAfter); }

}}
