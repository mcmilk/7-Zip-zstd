// Archive/GZipOut.h

#pragma once

#ifndef __ARCHIVE_GZIP_OUT_H
#define __ARCHIVE_GZIP_OUT_H

#include "Common/MyCom.h"
#include "GZipHeader.h"
#include "GZipItem.h"
#include "../../IStream.h"

namespace NArchive {
namespace NGZip {

class COutArchive
{
  CMyComPtr<IOutStream> m_Stream;
  HRESULT WriteBytes(const void *buffer, UINT32 size);
public:
  void Create(IOutStream *outStream) {  m_Stream = outStream; }
  HRESULT WriteHeader(const CItem &item);
  HRESULT WritePostInfo(UINT32 crc, UINT32 anUnpackSize);
};

}}

#endif
