// Archive/LzmaIn.h

#ifndef __ARCHIVE_LZMA_IN_H
#define __ARCHIVE_LZMA_IN_H

#include "LzmaItem.h"
#include "../../IStream.h"

namespace NArchive {
namespace NLzma {

HRESULT ReadStreamHeader(ISequentialInStream *inStream, CHeader &st);

}}
  
#endif
