// CompressEngine.h

#pragma once

#ifndef __COMPRESSENGINE_H
#define __COMPRESSENGINE_H

#include "Common/String.h"
#include "../Common/CompressEngineCommon.h"

#include "CompressionMethodUtils.h"
#include "../Common/IArchiveHandler2.h"


HRESULT Compress(
    const NUpdateArchive::CActionSet &actionSet, 
    IArchiveHandler200 *archive,
    const CCompressionMethodMode &compressionMethod,
    const CSysString &archiveName,     
    const CArchiveItemInfoVector &archiveItems,
    const CArchiveStyleDirItemInfoVector &dirItems,
    bool enablePercents, 
    bool sfxMode,
    const CSysString &sfxModule);

#endif
