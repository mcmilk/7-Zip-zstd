// Archive/Rar/Headers.cpp

#include "StdAfx.h"

#include "RarHeader.h"

namespace NArchive{
namespace NRar{
namespace NHeader{

Byte kMarker[kMarkerSize] = {0x52 + 1, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00};
  
class CMarkerInitializer
{
public:
  CMarkerInitializer() { kMarker[0]--; };
};

static CMarkerInitializer markerInitializer;

}}}
