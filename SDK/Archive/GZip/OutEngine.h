// Archive/GZip/OutEngine.h

#pragma once

#ifndef __ARCHIVE_GZIP_OUTENGINE_H
#define __ARCHIVE_GZIP_OUTENGINE_H

#include "Archive/GZip/Header.h"
#include "Interface/IInOutStreams.h"
#include "Archive/GZip/ItemInfo.h"

namespace NArchive {
namespace NGZip {

class COutArchive
{
  CComPtr<IOutStream> m_Stream;
  HRESULT WriteBytes(const void *aBuffer, UINT32 aSize);
public:
  void Create(IOutStream *aStream);
  HRESULT WriteHeader(const CItemInfo &anItemInfo);
  HRESULT WritePostInfo(UINT32 aCRC, UINT32 anUnpackSize);
};

}}

#endif
