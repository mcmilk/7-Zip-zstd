// DebRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "DebHandler.h"
static IInArchive *CreateArc() { return new NArchive::NDeb::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Deb", L"deb", 0, 0xEC, { '!', '<', 'a', 'r', 'c', 'h', '>', '\n'  }, 8, false, CreateArc, 0 };

REGISTER_ARC(Deb)
