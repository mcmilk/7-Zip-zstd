#include "StdAfx.h"

#include "Alien/Compress/BWT/BZip2/bzlib.h"

extern "C"

void bz_internal_error (int errcode)
{
  throw "error";
}