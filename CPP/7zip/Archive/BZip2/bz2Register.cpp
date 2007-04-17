// BZip2Register.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "BZip2Handler.h"
static IInArchive *CreateArc() { return new NArchive::NBZip2::CHandler;  }
#ifndef EXTRACT_ONLY
static IOutArchive *CreateArcOut() { return new NArchive::NBZip2::CHandler;  }
#else
#define CreateArcOut 0
#endif

static CArcInfo g_ArcInfo =
  { L"BZip2", L"bz2 bzip2 tbz2 tbz", L"* * .tar .tar", 2, { 'B', 'Z', 'h' }, 3, true, CreateArc, CreateArcOut };

REGISTER_ARC(BZip2)
