// RpmRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "RpmHandler.h"
static IInArchive *CreateArc() { return new NArchive::NRpm::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Rpm", L"rpm", L".cpio.gz", 0xEB, { 0}, 0, false, CreateArc, 0 };

REGISTER_ARC(Rpm)
