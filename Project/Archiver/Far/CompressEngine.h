// CompressEngine.h

#pragma once

#ifndef __COMPRESSENGINE_H
#define __COMPRESSENGINE_H

#include "Common/String.h"
#include "../../Archiver/Common/CompressEngineCommon.h"
#include "ProxyHandler.h"

#include "Far/ProgressBox.h"


HRESULT Compress(const CSysStringVector &aFileNames, 
    const UString &anArchiveNamePrefix, 
    const NUpdateArchive::CActionSet &anActionSet, CProxyHandler *aProxyHandler,
    const CLSID &aClassID, bool aStoreMode, bool aMaximizeRatioMode,
    CSysString &anArchiveName, CProgressBox *aProgressBox);

// void CompressArchive(const CSysStringVector &aFileNames);

#endif
