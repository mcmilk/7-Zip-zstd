// ChmRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "ChmHandler.h"
static IInArchive *CreateArc() { return new NArchive::NChm::CHandler; }

static CArcInfo g_ArcInfo =
  { L"Chm", L"chm chi chq chw hxs hxi hxr hxq hxw lit", 0, 0xE9, { 'I', 'T', 'S', 'F' }, 4, false, CreateArc, 0 };

REGISTER_ARC(Chm)
