// Archive/Tar/OutEngine.h

#pragma once

#ifndef __ARCHIVE_TAR_OUTENGINE_H
#define __ARCHIVE_TAR_OUTENGINE_H

#include "Archive/Tar/ItemInfo.h"

#include "Interface/IInOutStreams.h"

namespace NArchive {
namespace NTar {

class COutArchive
{
  CComPtr<ISequentialOutStream> m_Stream;
  HRESULT WriteBytes(void *aBuffer, UINT32 aSize);
public:
  void Create(ISequentialOutStream *aStream);
  HRESULT WriteHeader(const CItemInfo &anItemInfo);
  HRESULT FillDataResidual(UINT64 aDataSize);
  HRESULT WriteFinishHeader();
};

}}

#endif
