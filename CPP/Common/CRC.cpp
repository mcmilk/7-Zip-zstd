// Common/CRC.cpp

#include "StdAfx.h"

extern "C" 
{ 
#include "../../C/7zCrc.h"
}

struct CCRCTableInit { CCRCTableInit() { CrcGenerateTable(); } } g_CRCTableInit;
