// Archive/GZip/Header.h

#include "StdAfx.h"

#include "GZipHeader.h"

namespace NArchive {
namespace NGZip {

extern UInt16 kSignature = 0x8B1F + 1;

static class CMarkersInitializer
{
public:
  CMarkersInitializer() 
    { kSignature--; }
} g_MarkerInitializer;

}}

