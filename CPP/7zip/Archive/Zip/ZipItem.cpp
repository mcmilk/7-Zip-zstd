// Archive/ZipItem.cpp

#include "StdAfx.h"

#include "ZipHeader.h"
#include "ZipItem.h"
#include "../Common/ItemNameUtils.h"
#include "../../../../C/CpuArch.h"

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

bool CExtraSubBlock::ExtractNtfsTime(int index, FILETIME &ft) const
{
  ft.dwHighDateTime = ft.dwLowDateTime = 0;
  UInt32 size = (UInt32)Data.GetCapacity();
  if (ID != NFileHeader::NExtraID::kNTFS || size < 32)
    return false;
  const Byte *p = (const Byte *)Data;
  p += 4; // for reserved
  size -= 4;
  while (size > 4)
  {
    UInt16 tag = GetUi16(p);
    UInt32 attrSize = GetUi16(p + 2);
    p += 4;
    size -= 4;
    if (attrSize > size)
      attrSize = size;
    
    if (tag == NFileHeader::NNtfsExtra::kTagTime && attrSize >= 24)
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

bool CExtraSubBlock::ExtractUnixTime(int index, UInt32 &res) const
{
  res = 0;
  UInt32 size = (UInt32)Data.GetCapacity();
  if (ID != NFileHeader::NExtraID::kUnixTime || size < 5)
    return false;
  const Byte *p = (const Byte *)Data;
  Byte flags = *p++;
  size--;
  for (int i = 0; i < 3; i++)
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
  WORD highAttributes = WORD((ExternalAttributes >> 16 ) & 0xFFFF);
  switch(MadeByVersion.HostOS)
  {
    case NFileHeader::NHostOS::kAMIGA:
      switch (highAttributes & NFileHeader::NAmigaAttribute::kIFMT)
      {
        case NFileHeader::NAmigaAttribute::kIFDIR: return true;
        case NFileHeader::NAmigaAttribute::kIFREG: return false;
        default: return false; // change it throw kUnknownAttributes;
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
  if (IsDir())
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
  if (IsDir())       // test it;
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
void CLocalItem::SetUtf8(bool isUtf8)
  { SetBitMask(NFileHeader::NFlags::kUtf8, isUtf8); }

}}
