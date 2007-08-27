// RarRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "RarHandler.h"
static IInArchive *CreateArc() { return new NArchive::NRar::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Rar", L"rar r00", 0, 3, {0x52 , 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00}, 7, false, CreateArc, 0,  };

REGISTER_ARC(Rar)
