// Archive/GZipOut.h

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
  CMyComPtr<ISequentialOutStream> m_Stream;
  HRESULT WriteBytes(const void *buffer, UInt32 size);
  HRESULT WriteByte(Byte value);
  HRESULT WriteUInt16(UInt16 value);
  HRESULT WriteUInt32(UInt32 value);
public:
  void Create(ISequentialOutStream *outStream) {  m_Stream = outStream; }
  HRESULT WriteHeader(const CItem &item);
  HRESULT WritePostHeader(const CItem &item);
};

}}

#endif
