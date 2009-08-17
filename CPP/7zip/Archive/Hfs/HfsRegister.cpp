// HfsRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "HfsHandler.h"
static IInArchive *CreateArc() { return new NArchive::NHfs::CHandler; }

static CArcInfo g_ArcInfo =
  { L"HFS", L"hfs", 0, 0xE3, { 'H', '+', 0, 4 }, 4, false, CreateArc, 0 };

REGISTER_ARC(Hfs)
