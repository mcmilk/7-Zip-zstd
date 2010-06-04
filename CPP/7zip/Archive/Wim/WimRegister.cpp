// WimRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "WimHandler.h"
static IInArchive *CreateArc() { return new NArchive::NWim::CHandler; }
#ifndef EXTRACT_ONLY
static IOutArchive *CreateArcOut() { return new NArchive::NWim::COutHandler; }
#else
#define CreateArcOut 0
#endif

static CArcInfo g_ArcInfo =
  { L"wim", L"wim swm", 0, 0xE6, { 'M', 'S', 'W', 'I', 'M', 0, 0, 0 }, 8, false, CreateArc, CreateArcOut };

REGISTER_ARC(Wim)
