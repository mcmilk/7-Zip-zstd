// ZipRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "ZipHandler.h"
static IInArchive *CreateArc() { return new NArchive::NZip::CHandler;  }
#ifndef EXTRACT_ONLY
static IOutArchive *CreateArcOut() { return new NArchive::NZip::CHandler;  }
#else
#define CreateArcOut 0
#endif

static CArcInfo g_ArcInfo =
  { L"Zip", L"zip jar xpi", 0, 1, { 0x50, 0x4B, 0x03, 0x04 }, 4, false, CreateArc, CreateArcOut };

REGISTER_ARC(Zip)
