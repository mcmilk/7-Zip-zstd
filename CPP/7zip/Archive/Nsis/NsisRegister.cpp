// NsisRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "NsisHandler.h"
static IInArchive *CreateArc() { return new NArchive::NNsis::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Nsis", 0, 0, 0x9, { 0xEF, 0xBE, 0xAD, 0xDE, 
0x4E, 0x75, 0x6C, 0x6C, 0x73, 0x6F, 0x66, 0x74, 0x49, 0x6E, 0x73, 0x74}, 16, false, CreateArc, 0 };

REGISTER_ARC(Nsis)
