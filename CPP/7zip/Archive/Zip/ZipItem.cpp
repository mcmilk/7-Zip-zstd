// Archive/ZipItem.cpp

#include "StdAfx.h"

#include "ZipHeader.h"
#include "ZipItem.h"
#include "../Common/ItemNameUtils.h"

namespace NArchive {
namespace NZip {

bool operator==(const CVersion &v1, const CVersion &v2)
{
  return (v1.Version == v2.Version) && (v1.HostOS == v2.HostOS);
}

bool operator!=(const CVersion &v1, const CVersion &v2)
{
  return !(v1 == v2);
} 

bool CLocalItem::IsImplodeBigDictionary() const
{ 
  if (CompressionMethod != NFileHeader::NCompressionMethod::kImploded)
    throw 12312212;
  return (Flags & NFileHeader::NFlags::kImplodeDictionarySizeMask) != 0; 
}

bool CLocalItem::IsImplodeLiteralsOn() const
{
  if (CompressionMethod != NFileHeader::NCompressionMethod::kImploded)
    throw 12312213;
  return (Flags & NFileHeader::NFlags::kImplodeLiteralsOnMask) != 0; 
}

bool CLocalItem::IsDirectory() const
{ 
  return NItemName::HasTailSlash(Name, GetCodePage());
}

bool CItem::IsDirectory() const
{ 
  if (NItemName::HasTailSlash(Name, GetCodePage()))
    return true;
  if (!FromCentral)
    return false;
  WORD highAttributes = WORD((ExternalAttributes >> 16 ) & 0xFFFF);
  switch(MadeByVersion.HostOS)
  {
    case NFileHeader::NHostOS::kAMIGA:
      switch (highAttributes & NFileHeader::NAmigaAttribute::kIFMT) 
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
      /*
      switch (highAttributes & NFileHeader::NUnixAttribute::kIFMT) 
      {
        case NFileHeader::NUnixAttribute::kIFDIR:
          return true;
        default:
          return false;
      }
      */
      return false;
  }
}

UInt32 CLocalItem::GetWinAttributes() const
{
  DWORD winAttributes = 0;
  if (IsDirectory())
    winAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  return winAttributes;
}

UInt32 CItem::GetWinAttributes() const
{
  DWORD winAttributes = 0;
  switch(MadeByVersion.HostOS)
  {
    case NFileHeader::NHostOS::kFAT:
    case NFileHeader::NHostOS::kNTFS:
      if (FromCentral)
        winAttributes = ExternalAttributes; 
      break;
    default:
      winAttributes = 0; // must be converted from unix value;
  }
  if (IsDirectory())       // test it;
    winAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  return winAttributes;
}

void CLocalItem::SetFlagBits(int startBitNumber, int numBits, int value)
{  
  UInt16 mask = (UInt16)(((1 << numBits) - 1) << startBitNumber);
  Flags &= ~mask;
  Flags |= value << startBitNumber;
}

void CLocalItem::SetBitMask(int bitMask, bool enable)
{  
  if(enable) 
    Flags |= bitMask;
  else
    Flags &= ~bitMask;
}

void CLocalItem::SetEncrypted(bool encrypted)
  { SetBitMask(NFileHeader::NFlags::kEncrypted, encrypted); }

}}
