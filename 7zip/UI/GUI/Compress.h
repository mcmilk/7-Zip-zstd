// GUI/Compress.h

#pragma once

#ifndef __GUI_COMPRESS_H
#define __GUI_COMPRESS_H

#include "Common/String.h"

HRESULT CompressArchive(
    const CSysString &archivePath, 
    const UStringVector &fileNames, 
    const UString &archiveType, 
    bool email,
    bool showDialog);

#endif
