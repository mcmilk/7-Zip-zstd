// Common/CRC.cpp

#include "StdAfx.h"

extern "C" 
{ 
#include "../../C/7zCrc.h"
}

class CCRCTableInit
{
public:
  CCRCTableInit() { CrcGenerateTable(); }
} g_CRCTableInit;
