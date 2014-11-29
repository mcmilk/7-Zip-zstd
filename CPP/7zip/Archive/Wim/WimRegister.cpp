// WimRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "WimHandler.h"

namespace NArchive {
namespace NWim {

IMP_CreateArcIn
IMP_CreateArcOut

static CArcInfo g_ArcInfo =
  { "wim", "wim swm", 0, 0xE6,
  8, { 'M', 'S', 'W', 'I', 'M', 0, 0, 0 },
  0,
  NArcInfoFlags::kAltStreams |
  NArcInfoFlags::kNtSecure |
  NArcInfoFlags::kSymLinks |
  NArcInfoFlags::kHardLinks
  , REF_CreateArc_Pair };

REGISTER_ARC(Wim)

}}
