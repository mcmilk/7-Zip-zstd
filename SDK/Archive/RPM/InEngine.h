// Archive/RPM/InEngine.h

#pragma once

#ifndef __ARCHIVE_RPM_INENGINE_H
#define __ARCHIVE_RPM_INENGINE_H

#include "Interface/IInOutStreams.h"

namespace NArchive {
namespace NRPM {
  
HRESULT OpenArchive(IInStream *aStream);
  
}}
  
#endif
