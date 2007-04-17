// CpioRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "CpioHandler.h"
static IInArchive *CreateArc() { return new NArchive::NCpio::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Cpio", L"cpio", 0, 0xED, { 0 }, 0, false, CreateArc, 0 };

REGISTER_ARC(Cpio)
