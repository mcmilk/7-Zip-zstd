// Archive/TarOut.h

#pragma once

#ifndef __ARCHIVE_TAR_OUT_H
#define __ARCHIVE_TAR_OUT_H

#include "TarItem.h"

#include "Common/MyCom.h"
#include "../../IStream.h"

namespace NArchive {
namespace NTar {

class COutArchive
{
  CMyComPtr<ISequentialOutStream> m_Stream;
  HRESULT WriteBytes(const void *buffer, UINT32 size);
public:
  void Create(ISequentialOutStream *outStream);
  HRESULT WriteHeaderReal(const CItem &item);
  HRESULT WriteHeader(const CItem &item);
  HRESULT FillDataResidual(UINT64 dataSize);
  HRESULT WriteFinishHeader();
};

}}

#endif
