// Archive/GZipOut.cpp

#include "StdAfx.h"

#include "GZipOut.h"
#include "Common/CRC.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NGZip {

HRESULT COutArchive::WriteBytes(const void *buffer, UINT32 size)
{
  UINT32 processedSize;
  RINOK(m_Stream->Write(buffer, size, &processedSize));
  if(processedSize != size)
    return E_FAIL;
  return S_OK;
}

HRESULT COutArchive::WriteHeader(const CItem &item)
{
  NFileHeader::CBlock header;
  header.Id = kSignature;
  header.CompressionMethod = item.CompressionMethod;
  header.Flags = item.Flags;
  header.Flags &= NFileHeader::NFlags::kNameIsPresent;
  header.Time = item.Time;
  header.ExtraFlags = item.ExtraFlags;
  header.HostOS = item.HostOS;
  RINOK(WriteBytes(&header, sizeof(header)));
  if (item.NameIsPresent())
  {
    RINOK(WriteBytes((LPCSTR)item.Name, item.Name.Length()));
    BYTE zero = 0;
    RINOK(WriteBytes(&zero, sizeof(zero)));
  }
  // check it
  return S_OK;
}

HRESULT COutArchive::WritePostInfo(UINT32 crc, UINT32 unpackSize)
{
  RINOK(WriteBytes(&crc, sizeof(crc)));
  return WriteBytes(&unpackSize, sizeof(unpackSize));
}

}}
