// Archive::Rar::ItemInfoEx.h

#pragma once

#ifndef __ARCHIVE_RAR_ITEMINFOEX_H
#define __ARCHIVE_RAR_ITEMINFOEX_H

#include "Archive/Rar/ItemInfo.h"

namespace NArchive{
namespace NRar{

class CItemInfoEx: public CItemInfo
{
public:
  UINT64 Position;
  UINT16  MainPartSize;
  UINT16  CommentSize;
  UINT64 GetFullSize()  const { return MainPartSize + CommentSize + PackSize; };
  //  DWORD GetHeaderWithCommentSize()  const { return MainPartSize + CommentSize; };
  UINT64 GetCommentPosition() const { return Position + MainPartSize; };
  UINT64 GetDataPosition()    const { return GetCommentPosition() + CommentSize; };
};

}}

#endif



