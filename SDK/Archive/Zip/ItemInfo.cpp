// Archive/Zip/ItemInfo.cpp

#include "StdAfx.h"

#include "Archive/Zip/Header.h"
#include "Archive/Zip/ItemInfo.h"
#include "Archive/Zip/ItemNameUtils.h"

namespace NArchive {
namespace NZip {

bool operator==(const CVersion &aVersion1, const CVersion &aVersion2)
{
  return (aVersion1.Version == aVersion2.Version) &&
    (aVersion1.HostOS == aVersion2.HostOS);
}

bool operator!=(const CVersion &aVersion1, const CVersion &aVersion2)
{
  return !(aVersion1 == aVersion2);
} 

bool CItemInfo::IsEncrypted() const
{ return (Flags & NFileHeader::NFlags::kEncryptedMask) != 0; }
bool CItemInfo::HasDescriptor() const
  { return (Flags & NFileHeader::NFlags::kDescriptorUsedMask) != 0; }

bool CItemInfo::IsImplodeBigDictionary() const
{ 
if (CompressionMethod != NFileHeader::NCompressionMethod::kImploded)
    throw 12312212;
  return (Flags & NFileHeader::NFlags::kImplodeDictionarySizeMask) != 0; 
}

bool CItemInfo::IsImplodeLiteralsOn() const
{
if (CompressionMethod != NFileHeader::NCompressionMethod::kImploded)
    throw 12312213;
  return (Flags & NFileHeader::NFlags::kImplodeLiteralsOnMask) != 0; 
}

static const char *kUnknownAttributes = "Unknown file attributes";

bool CItemInfo::IsDirectory() const
{ 
  if (NItemName::IsItDirName(Name))
    return true;
  WORD aHighAttributes = WORD((ExternalAttributes >> 16 ) & 0xFFFF);
  switch(MadeByVersion.HostOS)
  {
    case NFileHeader::NHostOS::kAMIGA:
      switch (aHighAttributes & NFileHeader::NAmigaAttribute::kIFMT) 
      {
        case NFileHeader::NAmigaAttribute::kIFDIR:
          return true;
        case NFileHeader::NAmigaAttribute::kIFREG:  
          return false;
        default:
          return false; // change it throw kUnknownAttributes;
      }
    case NFileHeader::NHostOS::kFAT:
    case NFileHeader::NHostOS::kNTFS:
    case NFileHeader::NHostOS::kHPFS:
    case NFileHeader::NHostOS::kVFAT:
      return ((ExternalAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    case NFileHeader::NHostOS::kAtari:
    case NFileHeader::NHostOS::kMac:
    case NFileHeader::NHostOS::kVMS:
    case NFileHeader::NHostOS::kVM_CMS:
    case NFileHeader::NHostOS::kAcorn:
    case NFileHeader::NHostOS::kMVS:
      return false; // change it throw kUnknownAttributes;
    default:
      switch (aHighAttributes & NFileHeader::NUnixAttribute::kIFMT) 
      {
        case NFileHeader::NUnixAttribute::kIFDIR:
          return true;
        default:
          return false;
      }
  }
}

UINT32 CItemInfo::GetWinAttributes() const
{
  DWORD aWinAtrribute;
  switch(MadeByVersion.HostOS)
  {
    case NFileHeader::NHostOS::kFAT:
    case NFileHeader::NHostOS::kNTFS:
      aWinAtrribute = ExternalAttributes; 
      break;
    default:
      aWinAtrribute = 0; // must be converted from unix value;; 
  }
  if (IsDirectory())       // test it;
    aWinAtrribute |= FILE_ATTRIBUTE_DIRECTORY;
  return aWinAtrribute;
}

void CItemInfo::ClearFlags()
  { Flags = 0; }

void CItemInfo::SetFlagBits(int aStartBitNumber, int aNumBits, int aValue)
{  
  WORD aMask = ((1 << aNumBits) - 1) << aStartBitNumber;
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

void CItemInfo::SetEncrypted(bool anEncrypted)
  { SetBitMask(NFileHeader::NFlags::kEncryptedMask, anEncrypted); }

}}
