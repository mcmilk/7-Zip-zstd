// Archive::Rar::Headers.cpp

#include "StdAfx.h"

#include "Archive/Rar/Header.h"
#include "Common/CRC.h"

static void UpdateCRCBytesWithoutStartBytes(CCRC &aCRC, const void *aBytes, 
    UINT32 aSize, UINT32 anExludeSize)
{
  aCRC.Update(((const BYTE *)aBytes) + anExludeSize, aSize - anExludeSize);
}

namespace NArchive{
namespace NRar{
namespace NHeader{

BYTE kMarker[kMarkerSize] = {0x52 + 1, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00};
  
class CMarkerInitializer
{
public:
  CMarkerInitializer() { kMarker[0]--; };
};

static CMarkerInitializer aMarkerInitializer;

namespace NArchive
{
  UINT16 CBlock::GetRealCRC() const
  {
    CCRC aCRC;
    UpdateCRCBytesWithoutStartBytes(aCRC, this, 
      sizeof(*this), sizeof(CRC));
    return UINT16(aCRC.GetDigest());
  }
}

namespace NFile
{
  UINT16 CBlock32::GetRealCRC(const void *aName, UINT32 aNameSize, 
      bool anExtraDataDefined, BYTE *anExtraData) const
  {
    CCRC aCRC;
    UpdateCRCBytesWithoutStartBytes(aCRC, this, 
      sizeof(*this), sizeof(HeadCRC));
    aCRC.Update(aName, aNameSize);
    if (anExtraDataDefined)
      aCRC.Update(anExtraData, 8);
    return UINT16(aCRC.GetDigest());
  }
  UINT16 CBlock64::GetRealCRC(const void *aName, UINT32 aNameSize) const
  {
    CCRC aCRC;
    UpdateCRCBytesWithoutStartBytes(aCRC, this, 
      sizeof(*this), sizeof(HeadCRC));
    aCRC.Update(aName, aNameSize);
    return UINT16(aCRC.GetDigest());
  }
}

}}}