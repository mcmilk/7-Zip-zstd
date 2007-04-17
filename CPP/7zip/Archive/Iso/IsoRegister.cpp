// IsoRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "IsoHandler.h"
static IInArchive *CreateArc() { return new NArchive::NIso::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Iso", L"iso", 0, 0xE7, { 'C', 'D', '0', '0', '1', 0x1 }, 7, false, CreateArc, 0 };

REGISTER_ARC(Iso)
