// UpdateSolidEngine.cpp

#include "StdAfx.h"

#include "UpdateSolidEngine.h"
#include "UpdateEngine.h"

#include "Compression/CopyCoder.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"
#include "Interface/StreamObjects.h"

#include "../Common/InStreamWithCRC.h"

#include "Archive/Common/ItemNameUtils.h"

#include "Windows/Defs.h"

#include "Handler.h"

#include "Util/InOutTempBuffer.h"

#include "FolderInStream.h"

#include "Encode.h"

using namespace std;

namespace NArchive {
namespace N7z {

static const kOneItemComplexity = 30;

struct CRefItem
{
  UINT32 Index;
  const CUpdateItemInfo *Data;
  UINT32 ExtensionPos;
  UINT32 NamePos;
};

static int CompareUpdateItems(const void *p1, const void *p2)
{
  const CRefItem &a1 = *((CRefItem *)p1);
  const CRefItem &a2 = *((CRefItem *)p2);
  const CUpdateItemInfo &u1 = *a1.Data;
  const CUpdateItemInfo &u2 = *a2.Data;
  int n;
  if (u1.IsDirectory != u2.IsDirectory)
  {
    if (u1.IsDirectory)
      return u1.IsAnti ? 1: -1;
    return u2.IsAnti ? -1: 1;
  }
  if (u1.IsDirectory)
  {
    if (u1.IsAnti != u2.IsAnti)
      return (u1.IsAnti ? 1 : -1);
    n = _wcsicmp(u1.Name, u2.Name);
    return (u1.IsAnti ? (-n) : n);
  }
  if((n = _wcsicmp(u1.Name + a1.ExtensionPos, u2.Name + a2.ExtensionPos)) != 0)
    return n;
  if((n = _wcsicmp(u1.Name + a1.NamePos, u2.Name + a2.NamePos)) != 0)
    return n;
  // if((n = MyCompare(u1.LastWriteTime, u2.LastWriteTime)) != 0)

  if (u1.LastWriteTimeIsDefined && u2.LastWriteTimeIsDefined)
    if((n = CompareFileTime(&u1.LastWriteTime, &u2.LastWriteTime)) != 0)
      return n;
  if (!u1.IsDirectory)
  {
    if((n = MyCompare(u1.Size, u2.Size)) != 0)
      return n;
  }
  return 0;
}

HRESULT UpdateSolidStd(NArchive::N7z::COutArchive &archive, 
    IInStream *inStream,
    const CCompressionMethodMode *method, 
    const CCompressionMethodMode *headerMethod, 
    const NArchive::N7z::CArchiveDatabaseEx &database,
    const CRecordVector<bool> &compressStatuses,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    const CRecordVector<UINT32> &copyIndices,
    IUpdateCallBack *updateCallback)
{
  if (copyIndices.Size() != 0)
    return E_FAIL;

  /*
  if (inStream != NULL)
    return E_FAIL;
  */

  RETURN_IF_NOT_S_OK(archive.SkeepPrefixArchiveHeader());

  UINT64 complexity = 0;
  UINT32 compressIndex = 0, copyIndexIndex = 0;
  
  CArchiveDatabase newDatabase;

  bool thereIsPackStream = false;
  int i;
  for(i = 0; i < compressStatuses.Size(); i++)
  {
    if (compressStatuses[i])
    {
      const CUpdateItemInfo &updateItem = updateItems[compressIndex++];
      complexity += updateItem.Size;
      if (updateItem.Commented)
        complexity += updateItem.CommentRange.Size;
    }
    else
    {
      return E_FAIL;
    }
  }

  RETURN_IF_NOT_S_OK(updateCallback->SetTotal(complexity));
  UINT64 currentComplexity = 0;
  RETURN_IF_NOT_S_OK(updateCallback->SetCompleted(&currentComplexity));


  int numFiles = updateItems.Size();

  CRecordVector<CRefItem> refItems;
  refItems.Reserve(numFiles);
  for (i = 0; i < numFiles; i++)
  {
    const CUpdateItemInfo &updateItem = updateItems[i];
    CRefItem refItem;
    refItem.Index = i;
    refItem.Data = &updateItem;
    int slash1Pos = updateItem.Name.ReverseFind(L'\\');
    int slash2Pos = updateItem.Name.ReverseFind(L'/');
    int slashPos = MyMax(slash1Pos, slash2Pos);
    if (slashPos >= 0)
      refItem.NamePos = slashPos + 1;
    else
      refItem.NamePos = 0;
    int dotPos = updateItem.Name.ReverseFind(L'.');
    if (dotPos < 0 || (dotPos < slashPos && slashPos >= 0))
      refItem.ExtensionPos = updateItem.Name.Length();
    else 
      refItem.ExtensionPos = dotPos + 1;
    refItems.Add(refItem);
  }
  
  qsort(&refItems.Front(), refItems.Size(), sizeof(refItems[0]), CompareUpdateItems);

  CRecordVector<UINT32> indices;
  indices.Reserve(numFiles);
  for (i = 0; i < numFiles; i++)
  {
    UINT32 index = refItems[i].Index;
    indices.Add(index);
    const CUpdateItemInfo &updateItem = updateItems[index];
    CFileItemInfo fileItem;
    // ConvertUnicodeToUTF(NItemName::MakeLegalName(updateItem.Name), fileItem.Name); // test it
    fileItem.Name = NItemName::MakeLegalName(updateItem.Name);

    if (updateItem.AttributesAreDefined)
      fileItem.SetAttributes(updateItem.Attributes);

    // if (updateItem.CreationTimeIsDefined)
      // fileItem.SetCreationTime(anOperation.ItemInfo.CreationTime);
    
    if (updateItem.LastWriteTimeIsDefined)
      fileItem.SetLastWriteTime(updateItem.LastWriteTime);
    
    
    fileItem.UnPackSize = updateItem.Size;
    fileItem.IsDirectory = updateItem.IsDirectory;
    fileItem.IsAnti = updateItem.IsAnti;
    if (!fileItem.IsAnti && !fileItem.IsDirectory && updateItem.Size != 0)
      thereIsPackStream = true;
    newDatabase.Files.Add(fileItem);
  }

  if (thereIsPackStream)
  {
    CComObjectNoLock<CFolderInStream> *inStreamSpec = 
      new CComObjectNoLock<CFolderInStream>;
    CComPtr<ISequentialInStream> solidInStream(inStreamSpec);

    inStreamSpec->Init(updateCallback, &indices.Front(), numFiles);
    
    CEncoder encoder(method);
    CFolderItemInfo folderItem;

    CComObjectNoLock<CLocalProgress> *localProgressSpec = 
      new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
    localProgressSpec->Init(updateCallback, true);
    CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(localProgress, &currentComplexity, NULL);

    RETURN_IF_NOT_S_OK(encoder.Encode(solidInStream, 
        NULL,
        folderItem, 
        archive.m_Stream, newDatabase.PackSizes, compressProgress));

    // folderItem.NumFiles = numFiles;
    newDatabase.Folders.Add(folderItem);
    UINT32 numUnPackStreams = 0;
    for (i = 0; i < numFiles; i++)
    {
      CFileItemInfo &fileItem = newDatabase.Files[i];
      
      fileItem.FileCRC = inStreamSpec->CRCs[i];
      fileItem.FileCRCIsDefined = true;

      fileItem.UnPackSize = inStreamSpec->Sizes[i];
      if (!fileItem.IsAnti && !fileItem.IsDirectory && fileItem.UnPackSize != 0)
        numUnPackStreams++;
    }
    newDatabase.NumUnPackStreamsVector.Reserve(1);
    newDatabase.NumUnPackStreamsVector.Add(numUnPackStreams);
  }

  return archive.WriteDatabase(newDatabase, headerMethod);
}

}}
