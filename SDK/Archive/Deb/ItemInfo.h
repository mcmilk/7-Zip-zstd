// Archive/Deb/ItemInfo.h

#pragma once

#ifndef __ARCHIVE_DEB_ITEMINFO_H
#define __ARCHIVE_DEB_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive {
namespace NDeb {

class CItemInfo
{
public:
  AString Name;
  UINT64 Size;
  UINT32 ModificationTime;
  UINT32 Mode;
};

}}

#endif
