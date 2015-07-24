// Archive/ZipItem.cpp

#include "StdAfx.h"

#include "ZipHeader.h"
#include "ZipItem.h"
#include "../Common/ItemNameUtils.h"
#include "../../../../C/CpuArch.h"

namespace NArchive {
namespace NZip {

using namespace NFileHeader;

bool CExtraSubBlock::ExtractNtfsTime(unsigned index, FILETIME &ft) const
{
  ft.dwHighDateTime = ft.dwLowDateTime = 0;
  UInt32 size = (UInt32)Data.Size();
  if (ID != NExtraID::kNTFS || size < 32)
    return false;
  const Byte *p = (const Byte *)Data;
  p += 4; // for reserved
  size -= 4;
  while (size > 4)
  {
    UInt16 tag = GetUi16(p);
    unsigned attrSize = GetUi16(p + 2);
    p += 4;
    size -= 4;
    if (attrSize > size)
      attrSize = size;
    
    if (tag == NNtfsExtra::kTagTime && attrSize >= 24)
    {
      p += 8 * index;
      ft.dwLowDateTime = GetUi32(p);
      ft.dwHighDateTime = GetUi32(p + 4);
      return true;
    }
    p += attrSize;
    size -= attrSize;
  }
  return false;
}

bool CExtraSubBlock::ExtractUnixTime(bool isCentral, unsigned index, UInt32 &res) const
{
  res = 0;
  UInt32 size = (UInt32)Data.Size();
  if (ID != NExtraID::kUnixTime || size < 5)
    return false;
  const Byte *p = (const Byte *)Data;
  Byte flags = *p++;
  size--;
  if (isCentral)
  {
    if (index != NUnixTime::kMTime ||
        (flags & (1 << NUnixTime::kMTime)) == 0 ||
        size < 4)
      return false;
    res = GetUi32(p);
    return true;
  }
  for (unsigned i = 0; i < 3; i++)
    if ((flags & (1 << i)) != 0)
    {
      if (size < 4)
        return false;
      if (index == i)
      {
        res = GetUi32(p);
        return true;
      }
      p += 4;
      size -= 4;
    }
  return false;
}

bool CLocalItem::IsDir() const
{
  return NItemName::HasTailSlash(Name, GetCodePage());
}

bool CItem::IsDir() const
{
  if (NItemName::HasTailSlash(Name, GetCodePage()))
    return true;
  if (!FromCentral)
    return false;
  
  UInt16 highAttrib = (UInt16)((ExternalAttrib >> 16 ) & 0xFFFF);

  Byte hostOS = GetHostOS();
  switch (hostOS)
  {
    case NHostOS::kAMIGA:
      switch (highAttrib & NAmigaAttrib::kIFMT)
      {
        case NAmigaAttrib::kIFDIR: return true;
        case NAmigaAttrib::kIFREG: return false;
        default: return false; // change it throw kUnknownAttributes;
      }
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
    case NHostOS::kHPFS:
    case NHostOS::kVFAT:
      return ((ExternalAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
    case NHostOS::kAtari:
    case NHostOS::kMac:
    case NHostOS::kVMS:
    case NHostOS::kVM_CMS:
    case NHostOS::kAcorn:
    case NHostOS::kMVS:
      return false; // change it throw kUnknownAttributes;
    case NHostOS::kUnix:
      return ((highAttrib & NUnixAttrib::kIFMT) == NUnixAttrib::kIFDIR);
    default:
      return false;
  }
}

UInt32 CItem::GetWinAttrib() const
{
  UInt32 winAttrib = 0;
  switch (GetHostOS())
  {
    case NHostOS::kFAT:
    case NHostOS::kNTFS:
      if (FromCentral)
        winAttrib = ExternalAttrib;
      break;
    case NHostOS::kUnix:
      // do we need to clear 16 low bits in this case?
      if (FromCentral)
        winAttrib = ExternalAttrib & 0xFFFF0000;
      break;
  }
  if (IsDir()) // test it;
    winAttrib |= FILE_ATTRIBUTE_DIRECTORY;
  return winAttrib;
}

bool CItem::GetPosixAttrib(UInt32 &attrib) const
{
  // some archivers can store PosixAttrib in high 16 bits even with HostOS=FAT.
  if (FromCentral && GetHostOS() == NHostOS::kUnix)
  {
    attrib = ExternalAttrib >> 16;
    return (attrib != 0);
  }
  attrib = 0;
  if (IsDir())
    attrib = NUnixAttrib::kIFDIR;
  return false;
}

}}
