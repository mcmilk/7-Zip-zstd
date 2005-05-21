#include "StdAfx.h"

#include "Original/bzlib.h"

extern "C"

void bz_internal_error (int errcode)
{
  throw "error";
}