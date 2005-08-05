// Archive/Lzh/Header.h

#ifndef __ARCHIVE_LZH_HEADER_H
#define __ARCHIVE_LZH_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NLzh {

const int kMethodIdSize = 5;

const Byte kExtIdFileName = 0x01;
const Byte kExtIdDirName  = 0x02;
const Byte kExtIdUnixTime = 0x54;

}}

#endif
