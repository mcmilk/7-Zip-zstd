// UpdateMain2.cpp

#include "StdAfx.h"

#include "UpdateMain.h"

#include "Archive/Zip/InEngine.h"
#include "Archive/Zip/OutEngine.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace NZip {

HRESULT UpdateMain(    
    const NArchive::NZip::CItemInfoExVector &anInputItems,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    IOutStream *anOutStream,
    NArchive::NZip::CInArchive *anInArchive,
    CCompressionMethodMode *aCompressionMethodMode,
    IUpdateCallBack *anUpdateCallBack)
{
  DWORD aStartBlockSize;
  bool aCommentRangeAssigned;
  CUpdateRange aCommentRange;
  if(anInArchive != 0)
  {
    CInArchiveInfo anArchiveInfo;
    anInArchive->GetArchiveInfo(anArchiveInfo);
    aStartBlockSize = anArchiveInfo.StartPosition;
    aCommentRangeAssigned = anArchiveInfo.IsCommented();
    if (aCommentRangeAssigned)
    {
      aCommentRange.Position = anArchiveInfo.CommentPosition;
      aCommentRange.Size = anArchiveInfo.CommentSize;
    }
  }
  else
  {
    aStartBlockSize = 0;
    aCommentRangeAssigned = false;
  }
  
  COutArchive anOutArchive;
  anOutArchive.Create(anOutStream);
  if (aStartBlockSize > 0)
  {
    CComPtr<ISequentialInStream> anInStream;
    anInStream.Attach(anInArchive->CreateLimitedStream(0, aStartBlockSize));
    RETURN_IF_NOT_S_OK(CopyBlockToArchive(anInStream, anOutArchive, NULL));
    anOutArchive.MoveBasePosition(aStartBlockSize);
  }
  CComPtr<IInStream> anInStream;
  if(anInArchive != 0)
    anInStream.Attach(anInArchive->CreateStream());

  return UpdateArchiveStd(anOutArchive, anInStream, 
      anInputItems, aCompressStatuses, anUpdateItems, aCopyIndexes,
      aCompressionMethodMode, 
      aCommentRangeAssigned, aCommentRange, anUpdateCallBack);
}

}}
