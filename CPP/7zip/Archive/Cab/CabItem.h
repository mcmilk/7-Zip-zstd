// Archive/CabItem.h

#ifndef __ARCHIVE_CAB_ITEM_H
#define __ARCHIVE_CAB_ITEM_H

#include "Common/Types.h"
#include "Common/MyString.h"
#include "CabHeader.h"

namespace NArchive {
namespace NCab {

struct CFolder
{
  UInt32 DataStart; // offset of the first CFDATA block in this folder
  UInt16 NumDataBlocks; // number of CFDATA blocks in this folder
  Byte CompressionTypeMajor;
  Byte CompressionTypeMinor;
  Byte GetCompressionMethod() const { return (Byte)(CompressionTypeMajor & 0xF); }
};

class CItem
{
public:
  AString Name;
  UInt32 Offset;
  UInt32 Size;
  UInt32 Time;
  UInt16 FolderIndex;
  UInt16 Flags;
  UInt16 Attributes;
  UInt64 GetEndOffset() const { return (UInt64)Offset + Size; }
  UInt32 GetWinAttributes() const { return (Attributes & ~NHeader::kFileNameIsUTFAttributeMask); }
  bool IsNameUTF() const { return (Attributes & NHeader::kFileNameIsUTFAttributeMask) != 0; }
  bool IsDirectory() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }

  bool ContinuedFromPrev() const 
  { 
    return 
      (FolderIndex == NHeader::NFolderIndex::kContinuedFromPrev) ||
      (FolderIndex == NHeader::NFolderIndex::kContinuedPrevAndNext);
  }
  bool ContinuedToNext() const 
  { 
    return 
      (FolderIndex == NHeader::NFolderIndex::kContinuedToNext) ||
      (FolderIndex == NHeader::NFolderIndex::kContinuedPrevAndNext);
  }

  int GetFolderIndex(int numFolders) const 
  { 
    if (ContinuedFromPrev())
      return 0;
    if (ContinuedToNext())
      return (numFolders - 1);
    return FolderIndex;
  }
};

}}

#endif
