// 7z/Header.cpp

#include "StdAfx.h"
#include "Header.h"

namespace NArchive {
namespace N7z {

BYTE kSignature[kSignatureSize] = {'7' + 1, 'z', 0xBC, 0xAF, 0x27, 0x1C};

class SignatureInitializer
{
public:
  SignatureInitializer() { kSignature[0]--; };
} g_SignatureInitializer;

}}

