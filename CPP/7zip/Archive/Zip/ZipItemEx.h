// Archive/ZipItemEx.h

#ifndef __ARCHIVE_ZIP_ITEMEX_H
#define __ARCHIVE_ZIP_ITEMEX_H

#include "ZipHeader.h"
#include "ZipItem.h"

namespace NArchive {
namespace NZip {
  
class CItemEx: public CItem
{
public:
  UInt32 FileHeaderWithNameSize;
  UInt16 LocalExtraSize;
  
  UInt64 GetLocalFullSize() const
    { return FileHeaderWithNameSize + LocalExtraSize + PackSize +
      (HasDescriptor() ? NFileHeader::kDataDescriptorSize : 0); };
  /*
  UInt64 GetLocalFullSize(bool isZip64) const
    { return FileHeaderWithNameSize + LocalExtraSize + PackSize +
    (HasDescriptor() ? (isZip64 ? NFileHeader::kDataDescriptor64Size : NFileHeader::kDataDescriptorSize) : 0); };
  */
  UInt64 GetLocalExtraPosition() const
    { return LocalHeaderPosition + FileHeaderWithNameSize; };
  UInt64 GetDataPosition() const
    { return GetLocalExtraPosition() + LocalExtraSize; };
};

}}

#endif
