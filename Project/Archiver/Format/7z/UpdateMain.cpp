// UpdateMain.cpp

#include "StdAfx.h"

#include "UpdateMain.h"

#include "InEngine.h"
#include "OutEngine.h"

#include "Interface/LimitedStreams.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace N7z {

HRESULT UpdateMain(const NArchive::N7z::CArchiveDatabaseEx &database,
    CObjectVector<CUpdateItemInfo> &updateItems,
    IOutStream *outStream,
    IInStream *inStream,
    NArchive::N7z::CInArchiveInfo *inArchiveInfo,
    CCompressionMethodMode *method,
    CCompressionMethodMode *headerMethod,
    bool useAdditionalHeaderStreams, 
    bool compressMainHeader,
    IArchiveUpdateCallback *updateCallback,
    bool solid,
    bool removeSfxBlock)
{
  UINT64 startBlockSize;
  if(inArchiveInfo != 0)
    startBlockSize = inArchiveInfo->StartPosition;
  else
    startBlockSize = 0;
  
  if (startBlockSize > 0)
  {
    if (!removeSfxBlock)
    {
      CComObjectNoLock<CLimitedSequentialInStream> *streamSpec = new 
        CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<ISequentialInStream> limitedStream(streamSpec);
      RETURN_IF_NOT_S_OK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
      streamSpec->Init(inStream, startBlockSize);
      RETURN_IF_NOT_S_OK(CopyBlock(limitedStream, outStream, NULL));
    }
    else
    {
      // maybe need seek?
    }
  }


  COutArchive outArchive;
  outArchive.Create(outStream);
  if (solid)
    return UpdateSolidStd(outArchive, inStream, 
        method, headerMethod, useAdditionalHeaderStreams, compressMainHeader, 
        database, updateItems, updateCallback);
  else
    return UpdateArchiveStd(outArchive, inStream, 
        method, headerMethod, useAdditionalHeaderStreams, compressMainHeader,
        database, updateItems, updateCallback);
}

}}