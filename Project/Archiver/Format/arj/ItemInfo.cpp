// Archive/arj/ItemInfo.cpp

#include "StdAfx.h"

#include "Header.h"
#include "ItemInfo.h"

namespace NArchive {
namespace Narj {

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
{ return (Flags & NFileHeader::NFlags::kGarbled) != 0; }

static const char *kUnknownAttributes = "Unknown file attributes";

bool CItemInfo::IsDirectory() const
{ 
  return (FileType == NFileHeader::NFileType::kDirectory);
}

UINT32 CItemInfo::GetWinAttributes() const
{
  DWORD aWinAtrribute;
  switch(HostOS)
  {
    case NFileHeader::NHostOS::kMSDOS:
    case NFileHeader::NHostOS::kWIN95:
      aWinAtrribute = FileAccessMode;  // test it
      break;
    default:
      aWinAtrribute = 0; // must be converted from unix value;; 
  }
  if (IsDirectory())       // test it;
    aWinAtrribute |= FILE_ATTRIBUTE_DIRECTORY;
  return aWinAtrribute;
}

}}
