// Archive/Zip/ItemNameUtils.h

#pragma once

#ifndef __ARCHIVE_ZIP_ITEMNAMEUTILS_H
#define __ARCHIVE_ZIP_ITEMNAMEUTILS_H

#include "Common/String.h"

namespace NArchive {
namespace NZip {
namespace NItemName {

  bool IsNameLegal(const UString &aName);
  bool IsItDirName(const AString &aName);
  AString MakeLegalName(const AString &aName);
  AString MakeLegalDirName(const AString &aDirName);
  UString GetOSName(const UString &aZipDirName);

}}}


#endif