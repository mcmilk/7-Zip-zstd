// Archive/Common/ItemNameUtils.h

#pragma once

#ifndef __ARCHIVE_ITEMNAMEUTILS_H
#define __ARCHIVE_ITEMNAMEUTILS_H

#include "../../../Common/String.h"

namespace NArchive {
namespace NItemName {

  UString MakeLegalName(const UString &aName);
  UString GetOSName(const UString &aName);
  UString GetOSName2(const UString &aName);

}}

#endif
