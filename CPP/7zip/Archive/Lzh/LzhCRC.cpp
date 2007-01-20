// LzhCRC.cpp

#include "StdAfx.h"

#include "LzhCRC.h"

namespace NArchive {
namespace NLzh {

static const UInt16 kCRCPoly = 0xA001;

UInt16 CCRC::Table[256];

void CCRC::InitTable()
{
  for (UInt32 i = 0; i < 256; i++)
  {
    UInt32 r = i;
    for (int j = 0; j < 8; j++)
      if (r & 1) 
        r = (r >> 1) ^ kCRCPoly;
      else     
        r >>= 1;
    CCRC::Table[i] = (UInt16)r;
  }
}

class CCRCTableInit
{
public:
  CCRCTableInit() { CCRC::InitTable(); }
} g_CRCTableInit;

void CCRC::Update(const void *data, size_t size)
{
  UInt16 v = _value;
  const Byte *p = (const Byte *)data;
  for (; size > 0; size--, p++)
    v = (UInt16)(Table[((Byte)(v)) ^ *p] ^ (v >> 8));
  _value = v;
}

}}
