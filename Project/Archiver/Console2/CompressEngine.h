// CompressEngine.h

#pragma once

#ifndef __COMPRESSENGINE_H
#define __COMPRESSENGINE_H

#include "Common/String.h"
#include "../Common/CompressEngineCommon.h"

#include "CompressionMethodUtils.h"
#include "../Common/IArchiveHandler2.h"


HRESULT Compress(
    const NUpdateArchive::CActionSet &anActionSet, 
    IArchiveHandler200 *anArchive,
    const CCompressionMethodMode &aCompressionMethod,
    const CSysString &anArchiveName,     
    const CArchiveItemInfoVector &anArchiveItems,
    const CArchiveStyleDirItemInfoVector &aDirItems,
    bool anEnablePercents, 
    bool aSfxMode,
    const CSysString &aSfxModule);

#endif
