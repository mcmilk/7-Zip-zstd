// Archive/BZip2Item.h

#pragma once

#ifndef __ARCHIVE_BZIP2_ITEM_H
#define __ARCHIVE_BZIP2_ITEM_H

namespace NArchive {
namespace NBZip2 {
  
struct CItem
{
  UINT64 PackSize;
  UINT64 UnPackSize;
};

}}

#endif



