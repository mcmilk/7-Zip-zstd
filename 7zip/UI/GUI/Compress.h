// GUI/Compress.h

#pragma once

#ifndef __GUI_COMPRESS_H
#define __GUI_COMPRESS_H

#include "Common/String.h"

HRESULT CompressArchive(
    const CSysStringVector &fileNames, 
    const CSysString &archiveName, 
    bool email);

#endif
