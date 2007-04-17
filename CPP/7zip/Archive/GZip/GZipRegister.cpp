// GZipRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "GZipHandler.h"
static IInArchive *CreateArc() { return new NArchive::NGZip::CHandler;  }
#ifndef EXTRACT_ONLY
static IOutArchive *CreateArcOut() { return new NArchive::NGZip::CHandler;  }
#else
#define CreateArcOut 0
#endif

static CArcInfo g_ArcInfo =
  { L"GZip", L"gz gzip tgz tpz", L"* * .tar .tar", 0xEF, { 0x1F, 0x8B }, 2, true, CreateArc, CreateArcOut };

REGISTER_ARC(GZip)
