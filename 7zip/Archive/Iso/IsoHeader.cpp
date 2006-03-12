// Archive/Iso/Header.h

#include "StdAfx.h"

#include "IsoHeader.h"

namespace NArchive {
namespace NIso {

const char *kElToritoSpec = "EL TORITO SPECIFICATION\0\0\0\0\0\0\0\0\0";

const wchar_t *kMediaTypes[5] = 
{
  L"NoEmulation",
  L"1.2M",
  L"1.44M",
  L"2.88M",
  L"HardDisk"
};

}}
