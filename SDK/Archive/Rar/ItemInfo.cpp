// Archive::Rar::ItemInfo.cpp

#include "stdafx.h"

#include "Archive/Rar/ItemInfo.h"
#include "Archive/Rar/Header.h"

namespace NArchive{
namespace NRar{

bool CItemInfo::IsEncrypted() const
  { return (Flags & NHeader::NFile::kEncrypted) != 0; }
bool CItemInfo::IsSolid() const
  { return (Flags & NHeader::NFile::kSolid) != 0; }
bool CItemInfo::IsCommented() const
  { return (Flags & NHeader::NFile::kComment) != 0; }
bool CItemInfo::IsSplitBefore() const
  { return (Flags & NHeader::NFile::kSplitBefore) != 0; }
bool CItemInfo::IsSplitAfter() const
  { return (Flags & NHeader::NFile::kSplitAfter) != 0; }
bool CItemInfo::HasSalt() const
  { return (Flags & NHeader::NFile::kSalt) != 0; }
bool CItemInfo::HasUnicodeName() const
  { return (Flags & NHeader::NFile::kUnicodeName) != 0; }
bool CItemInfo::IsOldVersion() const
  { return (Flags & NHeader::NFile::kOldVersion) != 0; }

bool CItemInfo::IgnoreItem() const
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

UINT32  CItemInfo::GetDictSize() const
{ return (Flags >> NHeader::NFile::kDictBitStart) & NHeader::NFile::kDictMask; }

bool CItemInfo::IsDirectory() const
{ 
  return (GetDictSize() == NHeader::NFile::kDictDirectoryValue);
}

UINT32 CItemInfo::GetWinAttributes() const
{
  UINT32 aWinAttributes;
  switch(HostOS)
  {
  case NHeader::NFile::kHostMSDOS:
  case NHeader::NFile::kHostOS2:
  case NHeader::NFile::kHostWin32:
    aWinAttributes = Attributes; 
    break;
  default:
    aWinAttributes = 0; // must be converted from unix value;; 
  }
  if (IsDirectory())       // test it;
    aWinAttributes |= NHeader::NFile::kWinFileDirectoryAttributeMask;
  return aWinAttributes;
}

void CItemInfo::ClearFlags()
{ Flags = 0; }

void CItemInfo::SetFlagBits(int aStartBitNumber, int aNumBits, int aValue)
{  
  UINT16 aMask = ((1 << aNumBits) - 1) << aStartBitNumber;
  Flags &= ~aMask;
  Flags |= aValue << aStartBitNumber;
}

void CItemInfo::SetBitMask(int aBitMask, bool anEnable)
{  
  if(anEnable) 
    Flags |= aBitMask;
  else
    Flags &= ~aBitMask;
}

void CItemInfo::SetDictSize(UINT32 aSize)
{ 
  SetFlagBits(NHeader::NFile::kDictBitStart, NHeader::NFile::kNumDictBits, (aSize & NHeader::NFile::kDictMask));
}

void CItemInfo::SetAsDirectory(bool aDirectory)
{
  if (aDirectory)
    SetDictSize(NHeader::NFile::kDictDirectoryValue);
}

void CItemInfo::SetEncrypted(bool anEncrypted)
  { SetBitMask(NHeader::NFile::kEncrypted, anEncrypted); }
void CItemInfo::SetSolid(bool aSolid)
  { SetBitMask(NHeader::NFile::kSolid, aSolid); }
void CItemInfo::SetCommented(bool aCommented)
  { SetBitMask(NHeader::NFile::kComment, aCommented); }
void CItemInfo::SetSplitBefore(bool aSplitBefore)
  { SetBitMask(NHeader::NFile::kSplitBefore, aSplitBefore); }
void CItemInfo::SetSplitAfter(bool aSplitAfter)
  { SetBitMask(NHeader::NFile::kSplitAfter, aSplitAfter); }

}}
