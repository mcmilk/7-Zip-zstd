// IsoRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "IsoHandler.h"

namespace NArchive {
namespace NIso {

IMP_CreateArcIn

static CArcInfo g_ArcInfo =
  { "Iso", "iso img", 0, 0xE7,
  5, { 'C', 'D', '0', '0', '1' },
  NArchive::NIso::kStartPos + 1,
  0,
  CreateArc };

REGISTER_ARC(Iso)

}}
