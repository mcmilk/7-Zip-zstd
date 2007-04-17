// SplitRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "SplitHandler.h"
static IInArchive *CreateArc() { return new NArchive::NSplit::CHandler;  }
/*
#ifndef EXTRACT_ONLY
static IOutArchive *CreateArcOut() { return new NArchive::NSplit::CHandler;  }
#else
#define CreateArcOut 0
#endif
*/

static CArcInfo g_ArcInfo =
{ L"Split", L"001", 0, 0xEA, { 0 }, 0, false, CreateArc, 0 };

REGISTER_ARC(Split)
