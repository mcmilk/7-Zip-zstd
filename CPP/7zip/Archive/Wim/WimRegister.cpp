// WimRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "WimHandler.h"
static IInArchive *CreateArc() { return new NArchive::NWim::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Wim", L"wim swm", 0, 0xE6, { 'M', 'S', 'W', 'I', 'M', 0, 0, 0 }, 8, false, CreateArc, 0 };

REGISTER_ARC(Wim)
