// FindSignature.h

#ifndef __FINDSIGNATURE_H
#define __FINDSIGNATURE_H

#include "../../IStream.h"

HRESULT FindSignatureInStream(ISequentialInStream *stream,
    const Byte *signature, unsigned signatureSize,
    const UInt64 *limit, UInt64 &resPos);

#endif
