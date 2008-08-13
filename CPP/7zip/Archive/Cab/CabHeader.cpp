// Archive/Cab/Header.h

#include "StdAfx.h"

#include "CabHeader.h"

namespace NArchive{
namespace NCab{
namespace NHeader{

Byte kMarker[kMarkerSize] = {'M' + 1, 'S', 'C', 'F', 0, 0, 0, 0 };

struct SignatureInitializer { SignatureInitializer() {  kMarker[0]--;  }; } g_SignatureInitializer;

}}}
