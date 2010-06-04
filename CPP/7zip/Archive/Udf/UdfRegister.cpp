// UdfRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "UdfHandler.h"
static IInArchive *CreateArc() { return new NArchive::NUdf::CHandler; }

static CArcInfo g_ArcInfo =
  { L"Udf", L"iso img", 0, 0xE0, { 0, 'N', 'S', 'R', '0' }, 5, false, CreateArc, 0 };

REGISTER_ARC(Udf)
