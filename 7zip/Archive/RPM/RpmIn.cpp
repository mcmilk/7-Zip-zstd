// Archive/RpmIn.cpp

#include "StdAfx.h"

#include "RpmIn.h"

#include "RpmHeader.h"

#include "Windows/Defs.h"
#include "Common/MyCom.h"

namespace NArchive {
namespace NRpm {
 
HRESULT OpenArchive(IInStream *inStream)
{
  UINT64 pos;
  CLead lead;
  UINT32 processedSize;
  RINOK(inStream->Read(&lead, sizeof(lead), &processedSize));
  if (sizeof(lead) != processedSize)
    return S_FALSE;
  if (!lead.MagicCheck() || lead.Major < 3)
    return S_FALSE;
  lead.hton();

  CSigHeaderSig sigHeader, header;
  if(lead.SignatureType == RPMSIG_NONE)
  {
    ;
  }
  else if(lead.SignatureType == RPMSIG_PGP262_1024)
  {
    UINT64 pos;
    RINOK(inStream->Seek(256, STREAM_SEEK_CUR, &pos));
  }
  else if(lead.SignatureType == RPMSIG_HEADERSIG)
  {
    RINOK(inStream->Read(&sigHeader, sizeof(sigHeader), &processedSize));
    if (sizeof(sigHeader) != processedSize)
      return S_FALSE;
    if(!sigHeader.MagicCheck())
      return S_FALSE;
    sigHeader.hton();
    int len = sigHeader.GetLostHeaderLen();
    RINOK(inStream->Seek(len, STREAM_SEEK_CUR, &pos));
    if((pos % 8) != 0)
    {
      RINOK(inStream->Seek((pos / 8 + 1) * 8 - pos, 
          STREAM_SEEK_CUR, &pos));
    }
  }
  else
    return S_FALSE;

  RINOK(inStream->Read(&header, sizeof(header), &processedSize));
  if (sizeof(header) != processedSize)
    return S_FALSE;
  if(!header.MagicCheck())
    return S_FALSE;
  header.hton();
  int headerLen = header.GetLostHeaderLen();
  if(headerLen == -1)
    return S_FALSE;
  RINOK(inStream->Seek(headerLen, STREAM_SEEK_CUR, &pos));
  return S_OK;
}


}}
