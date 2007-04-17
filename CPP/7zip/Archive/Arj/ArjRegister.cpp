// ArjRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "ArjHandler.h"
static IInArchive *CreateArc() { return new NArchive::NArj::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Arj", L"arj", 0, 4, { 0x60, 0xEA }, 2, false, CreateArc, 0 };

REGISTER_ARC(Arj)
