// 7zRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "7zHandler.h"

namespace NArchive {
namespace N7z {

IMP_CreateArcIn
IMP_CreateArcOut

static CArcInfo g_ArcInfo =
  { "7z", "7z", 0, 7,
  6, {'7' + 1, 'z', 0xBC, 0xAF, 0x27, 0x1C},
  0,
  NArcInfoFlags::kFindSignature,
  REF_CreateArc_Pair };

REGISTER_ARC_DEC_SIG(7z)
// REGISTER_ARC(7z)

}}
