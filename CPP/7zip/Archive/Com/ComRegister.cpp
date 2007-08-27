// ComRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "ComHandler.h"
static IInArchive *CreateArc() { return new NArchive::NCom::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Compound", L"msi doc xls ppt", 0, 0xE5, { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 }, 8, false, CreateArc, 0 };

REGISTER_ARC(Com)
