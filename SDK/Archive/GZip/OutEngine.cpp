// Archive/GZip/OutEngine.cpp

#include "StdAfx.h"

#include "OutEngine.h"
#include "Common/StringConvert.h"
#include "Common/CRC.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NGZip {

void COutArchive::Create(IOutStream *aStream)
{
  m_Stream = aStream;
}

HRESULT COutArchive::WriteBytes(const void *aBuffer, UINT32 aSize)
{
  UINT32 aProcessedSize;
  RETURN_IF_NOT_S_OK(m_Stream->Write(aBuffer, aSize, &aProcessedSize));
  if(aProcessedSize != aSize)
    return E_FAIL;
  return S_OK;
}

HRESULT COutArchive::WriteHeader(const CItemInfo &anItemInfo)
{
  NFileHeader::CBlock aHeader;
  aHeader.Id = kSignature;
  aHeader.CompressionMethod = anItemInfo.CompressionMethod;
  aHeader.Flags = anItemInfo.Flags;
  aHeader.Flags &= NFileHeader::NFlags::kNameIsPresent;
  aHeader.Time = anItemInfo.Time;
  aHeader.ExtraFlags = anItemInfo.ExtraFlags;
  aHeader.HostOS = anItemInfo.HostOS;
  RETURN_IF_NOT_S_OK(WriteBytes(&aHeader, sizeof(aHeader)));
  if (anItemInfo.NameIsPresent())
  {
    RETURN_IF_NOT_S_OK(WriteBytes((LPCSTR)anItemInfo.Name, anItemInfo.Name.Length()));
    BYTE aZero = 0;
    RETURN_IF_NOT_S_OK(WriteBytes(&aZero, sizeof(aZero)));
  }
  return S_OK;
}

HRESULT COutArchive::WritePostInfo(UINT32 aCRC, UINT32 anUnpackSize)
{
  RETURN_IF_NOT_S_OK(WriteBytes(&aCRC, sizeof(aCRC)));
  return WriteBytes(&anUnpackSize, sizeof(anUnpackSize));
}

}}
