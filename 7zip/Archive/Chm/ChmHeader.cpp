// Archive/Chm/Header.h

#include "StdAfx.h"

#include "ChmHeader.h"

namespace NArchive{
namespace NChm{
namespace NHeader{

UInt32 kItsfSignature = 0x46535449 + 1;
UInt32 kItolSignature = 0x4C4F5449 + 1;
static class CSignatureInitializer
{ 
public:  
  CSignatureInitializer()
  { 
    kItsfSignature--; 
    kItolSignature--;
  }
}g_SignatureInitializer;


}}}
