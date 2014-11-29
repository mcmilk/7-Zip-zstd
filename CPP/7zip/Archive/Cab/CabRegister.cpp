// CabRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "CabHandler.h"

namespace NArchive {
namespace NCab {

IMP_CreateArcIn

static CArcInfo g_ArcInfo =
  { "Cab", "cab", 0, 8,
  8, { 'M', 'S', 'C', 'F', 0, 0, 0, 0 },
  0,
  NArcInfoFlags::kFindSignature,
  CreateArc };

REGISTER_ARC(Cab)

}}
