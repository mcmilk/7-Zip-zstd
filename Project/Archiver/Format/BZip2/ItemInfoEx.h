// Archive/BZip2/ItemInfoEx.h

#pragma once

#ifndef __ARCHIVE_BZIP2_ITEMINFOEX_H
#define __ARCHIVE_BZIP2_ITEMINFOEX_H

namespace NArchive {
namespace NBZip2 {
  
struct CItemInfo
{
  UINT64 PackSize;
  UINT64 UnPackSize;
};

struct CItemInfoEx: public CItemInfo
{
};

}}

#endif



