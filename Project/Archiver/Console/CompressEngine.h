// CompressEngine.h

#pragma once

#ifndef __COMPRESSENGINE_H
#define __COMPRESSENGINE_H

#include "Common/String.h"
#include "../Common/CompressEngineCommon.h"

#include "CompressionMethodUtils.h"
#include "../Format/Common/ArchiveInterface.h"


HRESULT Compress(
    const NUpdateArchive::CActionSet &actionSet, 
    IInArchive *archive,
    const CCompressionMethodMode &compressionMethod,
    const CSysString &archiveName,     
    const CArchiveItemInfoVector &archiveItems,
    const CArchiveStyleDirItemInfoVector &dirItems,
    bool enablePercents, 
    bool sfxMode,
    const CSysString &sfxModule);

#endif
