// Archive/RpmIn.h

#ifndef __ARCHIVE_RPM_IN_H
#define __ARCHIVE_RPM_IN_H

#include "../../IStream.h"

namespace NArchive {
namespace NRpm {
  
HRESULT OpenArchive(IInStream *inStream);
  
}}
  
#endif
