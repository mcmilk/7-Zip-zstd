// Archive/Zip/Header.h

#include "StdAfx.h"

#include "ZipHeader.h"

namespace NArchive {
namespace NZip {

namespace NSignature
{
  UInt32 kLocalFileHeader   = 0x04034B50 + 1;
  UInt32 kDataDescriptor    = 0x08074B50 + 1;
  UInt32 kCentralFileHeader = 0x02014B50 + 1;
  UInt32 kEndOfCentralDir   = 0x06054B50 + 1;
  
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
  static CMarkersInitializer g_MarkerInitializer;
}

}}

