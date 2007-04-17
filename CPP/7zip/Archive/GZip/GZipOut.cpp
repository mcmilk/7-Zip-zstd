// Archive/GZipOut.cpp

#include "StdAfx.h"

#include "GZipOut.h"

#include "Windows/Defs.h"
#include "../../Common/StreamUtils.h"

namespace NArchive {
namespace NGZip {

HRESULT COutArchive::WriteBytes(const void *buffer, UInt32 size)
{
  UInt32 processedSize;
  RINOK(WriteStream(m_Stream, buffer, size, &processedSize));
  if(processedSize != size)
    return E_FAIL;
  return S_OK;
}

HRESULT COutArchive::WriteByte(Byte value)
{
  return WriteBytes(&value, 1);
}

HRESULT COutArchive::WriteUInt16(UInt16 value)
{
  for (int i = 0; i < 2; i++)
  {
    RINOK(WriteByte((Byte)value));
    value >>= 8;
  }
  return S_OK;
}

HRESULT COutArchive::WriteUInt32(UInt32 value)
{
  for (int i = 0; i < 4; i++)
  {
    RINOK(WriteByte((Byte)value));
    value >>= 8;
  }
  return S_OK;
}

HRESULT COutArchive::WriteHeader(const CItem &item)
{
  RINOK(WriteUInt16(kSignature));
  RINOK(WriteByte(item.CompressionMethod));
  RINOK(WriteByte((Byte)(item.Flags & NFileHeader::NFlags::kNameIsPresent)));
  RINOK(WriteUInt32(item.Time));
  RINOK(WriteByte(item.ExtraFlags));
  RINOK(WriteByte(item.HostOS));
  if (item.NameIsPresent())
  {
    RINOK(WriteBytes((const char *)item.Name, item.Name.Length()));
    RINOK(WriteByte(0));
  }
  return S_OK;
}

HRESULT COutArchive::WritePostHeader(const CItem &item)
{
  RINOK(WriteUInt32(item.FileCRC));
  return WriteUInt32(item.UnPackSize32);
}

}}
