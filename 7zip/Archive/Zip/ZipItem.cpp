// Archive/ZipItem.cpp

#include "StdAfx.h"

#include "ZipHeader.h"
#include "ZipItem.h"

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

bool CItem::IsEncrypted() const
{ return (Flags & NFileHeader::NFlags::kEncryptedMask) != 0; }
bool CItem::HasDescriptor() const
  { return (Flags & NFileHeader::NFlags::kDescriptorUsedMask) != 0; }

bool CItem::IsImplodeBigDictionary() const
{ 
if (CompressionMethod != NFileHeader::NCompressionMethod::kImploded)
    throw 12312212;
  return (Flags & NFileHeader::NFlags::kImplodeDictionarySizeMask) != 0; 
}

bool CItem::IsImplodeLiteralsOn() const
{
if (CompressionMethod != NFileHeader::NCompressionMethod::kImploded)
    throw 12312213;
  return (Flags & NFileHeader::NFlags::kImplodeLiteralsOnMask) != 0; 
}

static const char *kUnknownAttributes = "Unknown file attributes";

bool CItem::IsDirectory() const
{ 
  if (!Name.IsEmpty())
  {
    #ifdef WIN32
      LPCSTR prev = CharPrevExA(GetCodePage(), Name, &Name[Name.Length()], 0);
      if (*prev == '/')
        return true;
    #else
      if (Name[Name.Length() - 1) == '/')
        return true;
    #endif
  }
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

UINT32 CItem::GetWinAttributes() const
{
  DWORD winAttributes;
  switch(MadeByVersion.HostOS)
  {
    case NFileHeader::NHostOS::kFAT:
    case NFileHeader::NHostOS::kNTFS:
      winAttributes = ExternalAttributes; 
      break;
    default:
      winAttributes = 0; // must be converted from unix value;; 
  }
  if (IsDirectory())       // test it;
    winAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  return winAttributes;
}

void CItem::ClearFlags()
  { Flags = 0; }

void CItem::SetFlagBits(int startBitNumber, int numBits, int value)
{  
  WORD mask = ((1 << numBits) - 1) << startBitNumber;
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

void CItem::SetEncrypted(bool encrypted)
  { SetBitMask(NFileHeader::NFlags::kEncryptedMask, encrypted); }

}}
