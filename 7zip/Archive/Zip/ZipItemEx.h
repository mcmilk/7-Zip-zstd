// Archive/ZipItemEx.h

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
  UInt16 FileHeaderWithNameSize;
  UInt64 CentralExtraPosition;
  
  UInt32 GetLocalFullSize()  const 
    { return FileHeaderWithNameSize + LocalExtraSize + PackSize + 
      (HasDescriptor() ? NFileHeader::kDataDescriptorSize : 0); };
  
  UInt32 GetCentralExtraPlusCommentSize()  const 
    { return CentralExtraSize + CommentSize; };
  
  UInt64 GetCommentPosition() const 
    { return CentralExtraPosition + CentralExtraSize; };
  
  bool IsCommented() const 
    { return CommentSize != 0; };
  
  UInt64 GetLocalExtraPosition() const 
    { return LocalHeaderPosition + FileHeaderWithNameSize; };
  
  UInt64 GetDataPosition() const 
    { return GetLocalExtraPosition() + LocalExtraSize; };
};

}}

#endif



