// Archive/RPM/Header.h

#pragma once

#ifndef __ARCHIVE_RPM_HEADER_H
#define __ARCHIVE_RPM_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NRPM {

#pragma pack(push, PragmaRPMHeaders)
#pragma pack(push, 1)

/* Reference: lib/signature.h of rpm package */
#define RPMSIG_NONE         0  /* Do not change! */
/* The following types are no longer generated */
#define RPMSIG_PGP262_1024  1  /* No longer generated */ /* 256 byte */
/* These are the new-style signatures.  They are Header structures.    */
/* Inside them we can put any number of any type of signature we like. */

#define RPMSIG_HEADERSIG    5  /* New Header style signature */

struct CLead
{
  unsigned char Magic[4];
  unsigned char Major;  /* not supported  ver1, only support 2,3 and lator */
  unsigned char Minor;
  short Type;
  short ArchNum;
  char Name[66];
  short OSNum;
  short SignatureType;
  char Reserved[16];  /* pad to 96 bytes -- 8 byte aligned */
  bool MagicCheck() const 
    { return Magic[0] == 0xed && Magic[1] == 0xab && Magic[2] == 0xee && Magic[3] == 0xdb; };
  static short my_htons(short s)
  { 
    const unsigned char *p = (const unsigned char*)&s; 
    return (short(p[0]) << 8) + (p[1]); }
  ;
  void hton()
  {
    Type = my_htons(Type);
    ArchNum = my_htons(ArchNum);
    OSNum = my_htons(OSNum);
    SignatureType = my_htons(SignatureType);
  };
};
  

struct CEntryInfo
{
  int Tag;
  int Type;
  int Offset; /* Offset from beginning of data segment, only defined on disk */
  int Count;
};

/* case: SignatureType == RPMSIG_HEADERSIG */
struct CSigHeaderSig
{
  unsigned char Magic[4];
  int Reserved;
  int IndexLen;  /* count of index entries */
  int DataLen;   /* number of bytes */
  int MagicCheck()
    { return Magic[0] == 0x8e && Magic[1] == 0xad && Magic[2] == 0xe8 && Magic[3] == 0x01; };
  int GetLostHeaderLen()
    { return IndexLen * sizeof(CEntryInfo) + DataLen;  };
  long my_htonl(long s)
  {
    unsigned char *p = (unsigned char*)&s;
    return (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
  };
  void hton()
  {
    IndexLen = my_htonl(IndexLen);
    DataLen = my_htonl(DataLen);
  };
};

#pragma pack(pop)
#pragma pack(pop, PragmaRPMHeaders)

}}

#endif
