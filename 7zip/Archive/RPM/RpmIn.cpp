// Archive/RpmIn.cpp

#include "StdAfx.h"

#include "RpmIn.h"

#include "RpmHeader.h"

#include "Windows/Defs.h"
#include "Common/MyCom.h"

#include "../../Common/StreamUtils.h"

namespace NArchive {
namespace NRpm {

static UInt16 GetUInt16(const char *data)
{
  return (UInt16)((Byte)data[1] | (((UInt16)(Byte)data[0]) << 8));
}

static UInt32 GetUInt32(const char *data)
{
  return 
      ((UInt32)(Byte)data[3]) |
      (((UInt32)(Byte)data[2]) << 8) |
      (((UInt32)(Byte)data[1]) << 16) |
      (((UInt32)(Byte)data[0]) << 24);
}

static HRESULT RedSigHeaderSig(IInStream *inStream, CSigHeaderSig &h)
{
  char dat[kCSigHeaderSigSize];
  char *cur = dat;
  UInt32 processedSize;
  RINOK(ReadStream(inStream, dat, kCSigHeaderSigSize, &processedSize));
  if (kCSigHeaderSigSize != processedSize)
    return S_FALSE;
  memmove(h.Magic, cur, 4);
  cur += 4;
  cur += 4;
  h.IndexLen = GetUInt32(cur);
  cur += 4;
  h.DataLen = GetUInt32(cur);
  return S_OK;
}

HRESULT OpenArchive(IInStream *inStream)
{
  UInt64 pos;
  char leadData[kLeadSize];
  char *cur = leadData;
  CLead lead;
  UInt32 processedSize;
  RINOK(ReadStream(inStream, leadData, kLeadSize, &processedSize));
  if (kLeadSize != processedSize)
    return S_FALSE;
  memmove(lead.Magic, cur, 4);
  cur += 4;
  lead.Major = *cur++;
  lead.Minor = *cur++;
  lead.Type = GetUInt16(cur);
  cur += 2;
  lead.ArchNum = GetUInt16(cur);
  cur += 2;
  memmove(lead.Name, cur, sizeof(lead.Name));
  cur += sizeof(lead.Name);
  lead.OSNum = GetUInt16(cur);
  cur += 2;
  lead.SignatureType = GetUInt16(cur);
  cur += 2;

  if (!lead.MagicCheck() || lead.Major < 3)
    return S_FALSE;

  CSigHeaderSig sigHeader, header;
  if(lead.SignatureType == RPMSIG_NONE)
  {
    ;
  }
  else if(lead.SignatureType == RPMSIG_PGP262_1024)
  {
    UInt64 pos;
    RINOK(inStream->Seek(256, STREAM_SEEK_CUR, &pos));
  }
  else if(lead.SignatureType == RPMSIG_HEADERSIG)
  {
    RINOK(RedSigHeaderSig(inStream, sigHeader));
    if(!sigHeader.MagicCheck())
      return S_FALSE;
    UInt32 len = sigHeader.GetLostHeaderLen();
    RINOK(inStream->Seek(len, STREAM_SEEK_CUR, &pos));
    if((pos % 8) != 0)
    {
      RINOK(inStream->Seek((pos / 8 + 1) * 8 - pos, 
          STREAM_SEEK_CUR, &pos));
    }
  }
  else
    return S_FALSE;

  RINOK(RedSigHeaderSig(inStream, header));
  if(!header.MagicCheck())
    return S_FALSE;
  int headerLen = header.GetLostHeaderLen();
  if(headerLen == -1)
    return S_FALSE;
  RINOK(inStream->Seek(headerLen, STREAM_SEEK_CUR, &pos));
  return S_OK;
}

}}
