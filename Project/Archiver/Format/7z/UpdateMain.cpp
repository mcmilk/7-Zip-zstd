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
    const CRecordVector<bool> &compressStatuses,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    const CRecordVector<UINT32> &copyIndices,
    IOutStream *outStream,
    IInStream *inStream,
    NArchive::N7z::CInArchiveInfo *inArchiveInfo,
    CCompressionMethodMode *method,
    CCompressionMethodMode *headerMethod,
    IUpdateCallBack *updateCallback,
    bool solid)
{
  UINT64 startBlockSize;
  if(inArchiveInfo != 0)
    startBlockSize = inArchiveInfo->StartPosition;
  else
    startBlockSize = 0;
  
  if (startBlockSize > 0)
  {
    CComObjectNoLock<CLimitedSequentialInStream> *streamSpec = new 
        CComObjectNoLock<CLimitedSequentialInStream>;
    CComPtr<ISequentialInStream> limitedStream(streamSpec);
    RETURN_IF_NOT_S_OK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    streamSpec->Init(inStream, startBlockSize);
    RETURN_IF_NOT_S_OK(CopyBlock(limitedStream, outStream, NULL));
  }

  COutArchive outArchive;
  outArchive.Create(outStream);
  if (solid)
    return UpdateSolidStd(outArchive, inStream, 
        method, headerMethod,
        database, compressStatuses,
        updateItems, copyIndices, updateCallback);
  else
    return UpdateArchiveStd(outArchive, inStream, 
        method, headerMethod,
        database, compressStatuses,
        updateItems, copyIndices, updateCallback);
}

}}