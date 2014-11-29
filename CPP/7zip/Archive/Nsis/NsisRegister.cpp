// NsisRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "NsisHandler.h"

namespace NArchive {
namespace NNsis {

IMP_CreateArcIn

static CArcInfo g_ArcInfo =
  { "Nsis", "nsis", 0, 0x9,
  NArchive::NNsis::kSignatureSize, NSIS_SIGNATURE,
  4,
  NArcInfoFlags::kFindSignature |
  NArcInfoFlags::kUseGlobalOffset,
  CreateArc };

REGISTER_ARC(Nsis)

}}
