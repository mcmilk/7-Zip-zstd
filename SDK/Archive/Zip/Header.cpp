// Archive/Zip/Header.h

#include "StdAfx.h"

#include "Archive/Zip/Header.h"
#include "Common/CRC.h"

static void UpdateCRCBytesWithoutStartBytes(CCRC &aCRC, const void *aBytes, 
    UINT32 aSize, UINT32 anExludeSize)
{
  aCRC.Update(((const BYTE *)aBytes) + anExludeSize, aSize - anExludeSize);
}

namespace NArchive {
namespace NZip {

namespace NSignature
{
  UINT32 kLocalFileHeader   = 0x04034B50 + 1;
  UINT32 kDataDescriptor    = 0x08074B50 + 1;
  UINT32 kCentralFileHeader = 0x02014B50 + 1;
  UINT32 kEndOfCentralDir   = 0x06054B50 + 1;
  
  class CMarkersInitializer
  {
  public:
    CMarkersInitializer() 
    { 
      kLocalFileHeader--; 
      kDataDescriptor--;
      kCentralFileHeader--;
      kEndOfCentralDir--;
    }
  };
  static CMarkersInitializer aMarkerInitializer;
}

}}

