// Archive::Rar::Headers.cpp

#include "StdAfx.h"

#include "RarHeader.h"
#include "Common/CRC.h"

static void UpdateCRCBytesWithoutStartBytes(CCRC &crc, const void *data, 
    UINT32 size, UINT32 exludeSize)
{
  crc.Update(((const BYTE *)data) + exludeSize, size - exludeSize);
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
    CCRC crc;
    UpdateCRCBytesWithoutStartBytes(crc, this, 
      sizeof(*this), sizeof(CRC));
    return UINT16(crc.GetDigest());
  }
}

namespace NFile
{
  /*
  UINT16 CBlock32::GetRealCRC(const void *aName, UINT32 aNameSize, 
      bool anExtraDataDefined, BYTE *anExtraData) const
  {
    CCRC crc;
    UpdateCRCBytesWithoutStartBytes(crc, this, 
      sizeof(*this), sizeof(HeadCRC));
    crc.Update(aName, aNameSize);
    if (anExtraDataDefined)
      crc.Update(anExtraData, 8);
    return UINT16(crc.GetDigest());
  }
  UINT16 CBlock64::GetRealCRC(const void *aName, UINT32 aNameSize) const
  {
    CCRC crc;
    UpdateCRCBytesWithoutStartBytes(crc, this, 
      sizeof(*this), sizeof(HeadCRC));
    crc.Update(aName, aNameSize);
    return UINT16(crc.GetDigest());
  }
  */
}

}}}