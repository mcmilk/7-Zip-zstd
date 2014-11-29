// RegisterArc.h

#ifndef __REGISTER_ARC_H
#define __REGISTER_ARC_H

#include "../Archive/IArchive.h"

struct CArcInfo
{
  const char *Name;
  const char *Ext;
  const char *AddExt;
  
  Byte ClassId;
  
  Byte SignatureSize;
  Byte Signature[20];
  UInt16 SignatureOffset;
  
  UInt16 Flags;

  Func_CreateInArchive CreateInArchive;
  Func_CreateOutArchive CreateOutArchive;
  Func_IsArc IsArc;

  bool IsMultiSignature() const { return (Flags & NArcInfoFlags::kMultiSignature) != 0; }
};

void RegisterArc(const CArcInfo *arcInfo) throw();

#define REGISTER_ARC_NAME(x) CRegister ## x

#define REGISTER_ARC(x) struct REGISTER_ARC_NAME(x) { \
    REGISTER_ARC_NAME(x)() { RegisterArc(&g_ArcInfo); }}; \
    static REGISTER_ARC_NAME(x) g_RegisterArc;

#define REGISTER_ARC_DEC_SIG(x) struct REGISTER_ARC_NAME(x) { \
    REGISTER_ARC_NAME(x)() { g_ArcInfo.Signature[0]--; RegisterArc(&g_ArcInfo); }}; \
    static REGISTER_ARC_NAME(x) g_RegisterArc;


#define IMP_CreateArcIn_2(c) \
  static IInArchive *CreateArc() { return new c; }

#define IMP_CreateArcIn IMP_CreateArcIn_2(CHandler)

#ifdef EXTRACT_ONLY
  #define IMP_CreateArcOut
  #define REF_CreateArc_Pair CreateArc, NULL
#else
  #define IMP_CreateArcOut static IOutArchive *CreateArcOut() { return new CHandler; }
  #define REF_CreateArc_Pair CreateArc, CreateArcOut
#endif

#endif
