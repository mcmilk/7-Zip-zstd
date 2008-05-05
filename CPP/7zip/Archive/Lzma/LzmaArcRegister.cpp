// LzmaArcRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "LzmaHandler.h"

static IInArchive *CreateArc() { return new NArchive::NLzma::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Lzma", L"lzma lzma86", 0, 0xA, {0 }, 0, true, CreateArc, NULL };

REGISTER_ARC(Lzma)
