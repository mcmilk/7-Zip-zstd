// Archive/Deb/Header.h

#pragma once

#ifndef __ARCHIVE_DEB_HEADER_H
#define __ARCHIVE_DEB_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NDeb {

#pragma pack( push, PragmaDebHeaders)
#pragma pack( push, 1)

namespace NHeader
{
  const int kNameSize = 16;
  const int kTimeSize = 12;
  const int kModeSize = 8;
  const int kSizeSize = 10;

  struct CHeader
  {
    char Name[kNameSize];
    char ModificationTime[kTimeSize];
    char Number0[6];
    char Number1[6];
    char Mode[kModeSize];
    char Size[kSizeSize];
    char Quote;
    char NewLine;
  };

  const int kSignatureLen = 8;
  extern const char *kSignature;
}

#pragma pack(pop)
#pragma pack(pop, PragmaDebHeaders)

}}

#endif
