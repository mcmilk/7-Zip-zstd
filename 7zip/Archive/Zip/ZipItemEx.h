// Archive/ZipItemEx.h

#pragma once

#ifndef __ARCHIVE_ZIP_ITEMEX_H
#define __ARCHIVE_ZIP_ITEMEX_H

#include "Common/Vector.h"

#include "ZipHeader.h"
#include "ZipItem.h"

namespace NArchive {
namespace NZip {
  
class CItemEx: public CItem
{
public:
  UINT16 FileHeaderWithNameSize;
  UINT64 CentralExtraPosition;
  
  UINT32 GetLocalFullSize()  const 
    { return FileHeaderWithNameSize + LocalExtraSize + PackSize + 
    (HasDescriptor() ? sizeof(NFileHeader::CDataDescriptor) : 0); };
  
  UINT32 GetCentralExtraPlusCommentSize()  const 
    { return CentralExtraSize + CommentSize; };
  
  UINT64 GetCommentPosition() const 
    { return CentralExtraPosition + CentralExtraSize; };
  
  bool IsCommented() const 
    { return CommentSize != 0; };
  
  UINT64 GetLocalExtraPosition() const 
    { return LocalHeaderPosition + FileHeaderWithNameSize; };
  
  UINT64 GetDataPosition() const 
    { return GetLocalExtraPosition() + LocalExtraSize; };
};

}}

#endif



