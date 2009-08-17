// NsisRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "NsisHandler.h"
static IInArchive *CreateArc() { return new NArchive::NNsis::CHandler; }

static CArcInfo g_ArcInfo =
  { L"Nsis", L"", 0, 0x9, NSIS_SIGNATURE, NArchive::NNsis::kSignatureSize, false, CreateArc, 0 };

REGISTER_ARC(Nsis)
