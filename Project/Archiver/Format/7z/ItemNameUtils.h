// 7z/ItemNameUtils.h

#pragma once

#ifndef __7Z_ITEMNAMEUTILS_H
#define __7Z_ITEMNAMEUTILS_H

#include "Common/String.h"

namespace NArchive {
namespace N7z {
namespace NItemName {

  UString MakeLegalName(const UString &aName);
  UString GetOSName(const UString &anInternalName);

}}}


#endif