// TarRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterArc.h"

#include "TarHandler.h"

namespace NArchive {
namespace NTar {

IMP_CreateArcIn
IMP_CreateArcOut

static CArcInfo g_ArcInfo =
  { "tar", "tar", 0, 0xEE,
  5, { 'u', 's', 't', 'a', 'r' },
  NFileHeader::kUstarMagic_Offset,
  NArcInfoFlags::kStartOpen |
  NArcInfoFlags::kSymLinks |
  NArcInfoFlags::kHardLinks,
  REF_CreateArc_Pair, IsArc_Tar };

REGISTER_ARC(Tar)

}}
