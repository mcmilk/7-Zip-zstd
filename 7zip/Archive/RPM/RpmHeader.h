// Archive/RpmHeader.h

#ifndef __ARCHIVE_RPM_HEADER_H
#define __ARCHIVE_RPM_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NRpm {

/* Reference: lib/signature.h of rpm package */
#define RPMSIG_NONE         0  /* Do not change! */
/* The following types are no longer generated */
#define RPMSIG_PGP262_1024  1  /* No longer generated */ /* 256 byte */
/* These are the new-style signatures.  They are Header structures.    */
/* Inside them we can put any number of any type of signature we like. */

#define RPMSIG_HEADERSIG    5  /* New Header style signature */

const UInt32 kLeadSize = 96;
struct CLead
{
  unsigned char Magic[4];
  unsigned char Major;  // not supported  ver1, only support 2,3 and lator
  unsigned char Minor;
  UInt16 Type;
  UInt16 ArchNum;
  char Name[66];
  UInt16 OSNum;
  UInt16 SignatureType;
  char Reserved[16];  // pad to 96 bytes -- 8 byte aligned
  bool MagicCheck() const 
    { return Magic[0] == 0xed && Magic[1] == 0xab && Magic[2] == 0xee && Magic[3] == 0xdb; };
};
  
const UInt32 kEntryInfoSize = 16;
/*
struct CEntryInfo
{
  int Tag;
  int Type;
  int Offset; // Offset from beginning of data segment, only defined on disk
  int Count;
};
*/

// case: SignatureType == RPMSIG_HEADERSIG
const UInt32 kCSigHeaderSigSize = 16;
struct CSigHeaderSig
{
  unsigned char Magic[4];
  UInt32 Reserved;
  UInt32 IndexLen;  // count of index entries
  UInt32 DataLen;   // number of bytes
  bool MagicCheck()
    { return Magic[0] == 0x8e && Magic[1] == 0xad && Magic[2] == 0xe8 && Magic[3] == 0x01; };
  UInt32 GetLostHeaderLen()
    { return IndexLen * kEntryInfoSize + DataLen;  };
};

}}

#endif
