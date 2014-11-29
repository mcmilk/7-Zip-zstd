// Archive/Iso/Header.h

#include "StdAfx.h"

#include "IsoHeader.h"

namespace NArchive {
namespace NIso {

const char *kElToritoSpec = "EL TORITO SPECIFICATION\0\0\0\0\0\0\0\0\0";

const char *kMediaTypes[5] =
{
    "NoEmulation"
  , "1.2M"
  , "1.44M"
  , "2.88M"
  , "HardDisk"
};

}}
