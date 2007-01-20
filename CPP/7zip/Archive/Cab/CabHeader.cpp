// Archive/Cab/Header.h

#include "StdAfx.h"

#include "CabHeader.h"

namespace NArchive{
namespace NCab{
namespace NHeader{

namespace NArchive {

UInt32 kSignature = 0x4643534d + 1;
static class CSignatureInitializer
{ public:  CSignatureInitializer() { kSignature--; }} g_SignatureInitializer;

}

}}}
