// BZip2CRC.cpp

#include "StdAfx.h"

#include "BZip2CRC.h"

UInt32 CBZip2CRC::Table[256];

static const UInt32 kBZip2CRCPoly = 0x04c11db7;  /* AUTODIN II, Ethernet, & FDDI */

void CBZip2CRC::InitTable()
{
  for (UInt32 i = 0; i < 256; i++)
  {
    UInt32 r = (i << 24);
    for (int j = 8; j > 0; j--)
      r = (r & 0x80000000) ? ((r << 1) ^ kBZip2CRCPoly) : (r << 1);
    Table[i] = r;
  }
}

class CBZip2CRCTableInit
{
public:
  CBZip2CRCTableInit() { CBZip2CRC::InitTable(); }
} g_BZip2CRCTableInit;
