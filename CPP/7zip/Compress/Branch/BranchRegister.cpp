// BranchRegister.cpp

#include "StdAfx.h"

#include "../../Common/RegisterCodec.h"

#include "PPC.h"
#include "IA64.h"
#include "ARM.h"
#include "ARMThumb.h"
#include "SPARC.h"

#define CREATE_CODEC(x) \
  static void *CreateCodec ## x() { return (void *)(ICompressFilter *)(new C ## x ## _Decoder); } \
  static void *CreateCodec ## x ## Out() { return (void *)(ICompressFilter *)(new C ## x ## _Encoder); }

CREATE_CODEC(BC_PPC_B)
CREATE_CODEC(BC_IA64)
CREATE_CODEC(BC_ARM)
CREATE_CODEC(BC_ARMThumb)
CREATE_CODEC(BC_SPARC)

#define METHOD_ITEM(x, id1, id2, name) { CreateCodec ## x, CreateCodec ## x ## Out, 0x03030000 + (id1 * 256) + id2, name, 1, true  }

static CCodecInfo g_CodecsInfo[] =
{
  METHOD_ITEM(BC_PPC_B,   0x02, 0x05, L"BC_PPC_B"),
  METHOD_ITEM(BC_IA64,    0x04, 1, L"BC_IA64"),
  METHOD_ITEM(BC_ARM,     0x05, 1, L"BC_ARM"),
  METHOD_ITEM(BC_ARMThumb,0x07, 1, L"BC_ARMThumb"),
  METHOD_ITEM(BC_SPARC,   0x08, 0x05, L"BC_SPARC")
};

REGISTER_CODECS(Branch)
