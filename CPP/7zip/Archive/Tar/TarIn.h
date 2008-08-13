// Archive/TarIn.h

#ifndef __ARCHIVE_TAR_IN_H
#define __ARCHIVE_TAR_IN_H

#include "Common/MyCom.h"
#include "../../IStream.h"

#include "TarItem.h"

namespace NArchive {
namespace NTar {
  
HRESULT ReadItem(ISequentialInStream *stream, bool &filled, CItemEx &itemInfo);

}}
  
#endif
