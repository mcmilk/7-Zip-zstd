// UpdateMain.cpp

#include "StdAfx.h"

#include "UpdateMain.h"

#include "InEngine.h"
#include "OutEngine.h"

#include "Interface/LimitedStreams.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace N7z {

HRESULT UpdateMain(const NArchive::N7z::CArchiveDatabaseEx &aDatabase,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    IOutStream *anOutStream,
    IInStream *anInStream,
    NArchive::N7z::CInArchiveInfo *anInArchiveInfo,
    CCompressionMethodMode *aMethod,
    CCompressionMethodMode *aHeaderMethod,
    IUpdateCallBack *anUpdateCallBack,
    bool aSolid)
{
  UINT64 aStartBlockSize;
  if(anInArchiveInfo != 0)
    aStartBlockSize = anInArchiveInfo->StartPosition;
  else
    aStartBlockSize = 0;
  
  if (aStartBlockSize > 0)
  {
    CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
        CComObjectNoLock<CLimitedSequentialInStream>;
    CComPtr<ISequentialInStream> aLimitedStream(aStreamSpec);
    RETURN_IF_NOT_S_OK(anInStream->Seek(0, STREAM_SEEK_SET, NULL));
    aStreamSpec->Init(anInStream, aStartBlockSize);
    RETURN_IF_NOT_S_OK(CopyBlock(aLimitedStream, anOutStream, NULL));
  }

  COutArchive anOutArchive;
  anOutArchive.Create(anOutStream);
  if (aSolid)
    return UpdateSolidStd(anOutArchive, anInStream, 
        aMethod, aHeaderMethod,
        aDatabase, aCompressStatuses,
        anUpdateItems, aCopyIndexes, anUpdateCallBack);
  else
    return UpdateArchiveStd(anOutArchive, anInStream, 
        aMethod, aHeaderMethod,
        aDatabase, aCompressStatuses,
        anUpdateItems, aCopyIndexes, anUpdateCallBack);
}

}}