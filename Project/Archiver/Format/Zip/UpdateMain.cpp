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
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    IOutStream *anOutStream,
    NArchive::NZip::CInArchive *anInArchive,
    CCompressionMethodMode *aCompressionMethodMode,
    IArchiveUpdateCallback *updateCallback)
{
  DWORD startBlockSize;
  bool commentRangeAssigned;
  CUpdateRange commentRange;
  if(anInArchive != 0)
  {
    CInArchiveInfo archiveInfo;
    anInArchive->GetArchiveInfo(archiveInfo);
    startBlockSize = archiveInfo.StartPosition;
    commentRangeAssigned = archiveInfo.IsCommented();
    if (commentRangeAssigned)
    {
      commentRange.Position = archiveInfo.CommentPosition;
      commentRange.Size = archiveInfo.CommentSize;
    }
  }
  else
  {
    startBlockSize = 0;
    commentRangeAssigned = false;
  }
  
  COutArchive anOutArchive;
  anOutArchive.Create(anOutStream);
  if (startBlockSize > 0)
  {
    CComPtr<ISequentialInStream> anInStream;
    anInStream.Attach(anInArchive->CreateLimitedStream(0, startBlockSize));
    RETURN_IF_NOT_S_OK(CopyBlockToArchive(anInStream, anOutArchive, NULL));
    anOutArchive.MoveBasePosition(startBlockSize);
  }
  CComPtr<IInStream> anInStream;
  if(anInArchive != 0)
    anInStream.Attach(anInArchive->CreateStream());

  return UpdateArchiveStd(anOutArchive, anInStream, 
      anInputItems, anUpdateItems, 
      aCompressionMethodMode, 
      commentRangeAssigned, commentRange, updateCallback);
}

}}
