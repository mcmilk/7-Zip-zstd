// Archive/RPM/InEngine.cpp

#include "StdAfx.h"

#include "InEngine.h"

#include "Archive/RPM/Header.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace NRPM {
 
HRESULT OpenArchive(IInStream *aStream)
{
  UINT64 aPos;
  CLead aLead;
  UINT32 aProcessedSize;
  RETURN_IF_NOT_S_OK(aStream->Read(&aLead, sizeof(aLead), &aProcessedSize));
  if (sizeof(aLead) != aProcessedSize)
    return S_FALSE;
  if (!aLead.MagicCheck() || aLead.Major < 3)
    return S_FALSE;
  aLead.hton();

  CSigHeaderSig aSigHeader, aHeader;
  if(aLead.SignatureType == RPMSIG_NONE)
  {
    ;
  }
  else if(aLead.SignatureType == RPMSIG_PGP262_1024)
  {
    UINT64 aPos;
    RETURN_IF_NOT_S_OK(aStream->Seek(256, STREAM_SEEK_CUR, &aPos));
  }
  else if(aLead.SignatureType == RPMSIG_HEADERSIG)
  {
    RETURN_IF_NOT_S_OK(aStream->Read(&aSigHeader, sizeof(aSigHeader), &aProcessedSize));
    if (sizeof(aSigHeader) != aProcessedSize)
      return S_FALSE;
    if(!aSigHeader.MagicCheck())
      return S_FALSE;
    aSigHeader.hton();
    int aLen = aSigHeader.GetLostHeaderLen();
    RETURN_IF_NOT_S_OK(aStream->Seek(aLen, STREAM_SEEK_CUR, &aPos));
    if((aPos % 8) != 0)
    {
      RETURN_IF_NOT_S_OK(aStream->Seek((aPos / 8 + 1) * 8 - aPos, 
          STREAM_SEEK_CUR, &aPos));
    }
  }
  else
    return S_FALSE;

  RETURN_IF_NOT_S_OK(aStream->Read(&aHeader, sizeof(aHeader), &aProcessedSize));
  if (sizeof(aHeader) != aProcessedSize)
    return S_FALSE;
  if(!aHeader.MagicCheck())
    return S_FALSE;
  aHeader.hton();
  int aHeaderLen = aHeader.GetLostHeaderLen();
  if(aHeaderLen == -1)
    return S_FALSE;
  RETURN_IF_NOT_S_OK(aStream->Seek(aHeaderLen, STREAM_SEEK_CUR, &aPos));
  return S_OK;
}


}}
