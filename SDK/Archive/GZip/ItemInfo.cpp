// Archive/GZip/ItemInfo.cpp

#include "StdAfx.h"

#include "Archive/GZip/Header.h"
#include "Archive/GZip/ItemInfo.h"

namespace NArchive {
namespace NGZip {

bool CItemInfo::IsText() const
  {  return TestFlag(NFileHeader::NFlags::kDataIsText); }
bool CItemInfo::HeaderCRCIsPresent() const
  {  return TestFlag(NFileHeader::NFlags::kHeaderCRCIsPresent); }
bool CItemInfo::ExtraFieldIsPresent() const
  {  return TestFlag(NFileHeader::NFlags::kExtraIsPresent); }
bool CItemInfo::NameIsPresent() const
  {  return TestFlag(NFileHeader::NFlags::kNameIsPresent); }
bool CItemInfo::CommentIsPresent() const
  {  return TestFlag(NFileHeader::NFlags::kComentIsPresent); }

void CItemInfo::SetNameIsPresentFlag(bool aNameIsPresent)
{  
  if (aNameIsPresent)
    Flags |= NFileHeader::NFlags::kNameIsPresent;
  else
    Flags &= (~NFileHeader::NFlags::kNameIsPresent);
}
  
}}
