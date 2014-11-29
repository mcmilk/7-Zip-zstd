// ZipRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "ZipHandler.h"

namespace NArchive {
namespace NZip {

IMP_CreateArcIn
IMP_CreateArcOut

static CArcInfo g_ArcInfo =
  { "zip", "zip zipx jar xpi odt ods docx xlsx epub", 0, 1,
  3 + 4 + 4 + 6,
  {
    4, 0x50, 0x4B, 0x03, 0x04,
    4, 0x50, 0x4B, 0x05, 0x06,
    6, 0x50, 0x4B, 0x30, 0x30, 0x50, 0x4B,
  },
  0,
  NArcInfoFlags::kFindSignature |
  NArcInfoFlags::kMultiSignature |
  NArcInfoFlags::kUseGlobalOffset,
  REF_CreateArc_Pair, IsArc_Zip };

REGISTER_ARC(Zip)

}}
