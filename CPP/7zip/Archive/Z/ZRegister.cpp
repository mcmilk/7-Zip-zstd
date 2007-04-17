// ZRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "ZHandler.h"
static IInArchive *CreateArc() { return new NArchive::NZ::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Z", L"z taz", L"* .tar", 5, { 0x1F, 0x9D }, 2, false, CreateArc, 0 };

REGISTER_ARC(Z)
