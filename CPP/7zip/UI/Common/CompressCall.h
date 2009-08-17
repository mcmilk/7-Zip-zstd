// CompressCall.h

#ifndef __COMPRESS_CALL_H
#define __COMPRESS_CALL_H

#include "Common/MyString.h"

UString GetQuotedString(const UString &s);

extern HWND g_HWND;
UString HResultToMessage(HRESULT errorCode);

HRESULT CompressFiles(
    const UString &arcPathPrefix,
    const UString &arcName,
    const UString &arcType,
    const UStringVector &names,
    bool email, bool showDialog, bool waitFinish);

HRESULT ExtractArchives(const UStringVector &arcPaths, const UString &outFolder, bool showDialog);
HRESULT TestArchives(const UStringVector &arcPaths);
HRESULT Benchmark();

#endif
