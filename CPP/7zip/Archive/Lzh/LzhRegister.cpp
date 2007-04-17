// LzhRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "LzhHandler.h"
static IInArchive *CreateArc() { return new NArchive::NLzh::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Lzh", L"lzh lha", 0, 6, { '-', 'l' }, 2, false, CreateArc, 0 };

REGISTER_ARC(Lzh)
