// Archive/Tar/ItemNameUtils.h

#pragma once

#ifndef __ARCHIVE_TAR_ITEMNAMEUTILS_H
#define __ARCHIVE_TAR_ITEMNAMEUTILS_H

#include "Common/String.h"

namespace NArchive {
namespace NTar {
namespace NItemName {

  bool IsNameLegal(const UString &aName);
  bool IsItDirName(const AString &aName);
  AString MakeLegalName(const AString &aName);
  AString MakeLegalDirName(const AString &aDirName);
  UString GetOSName(const UString &aZipDirName);

}}}


#endif