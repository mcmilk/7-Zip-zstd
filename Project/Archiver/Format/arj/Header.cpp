// Archive/arj/Header.h

#include "StdAfx.h"

#include "Header.h"
#include "Common/CRC.h"

static void UpdateCRCBytesWithoutStartBytes(CCRC &aCRC, const void *aBytes, 
    UINT32 aSize, UINT32 anExludeSize)
{
  aCRC.Update(((const BYTE *)aBytes) + anExludeSize, aSize - anExludeSize);
}

namespace NArchive {
namespace Narj {

namespace NSignature
{
}

}}

