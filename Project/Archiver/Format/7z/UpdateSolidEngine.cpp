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

#include "ItemNameUtils.h"

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
  if (u1.IsDirectory() != u2.IsDirectory())
  {
    if (u1.IsDirectory())
      return -1;
    return 1;
  }
  if((n = _wcsicmp(u1.Name + a1.ExtensionPos, u2.Name + a2.ExtensionPos)) != 0)
    return n;
  if((n = _wcsicmp(u1.Name + a1.NamePos, u2.Name + a2.NamePos)) != 0)
    return n;
  // if((n = MyCompare(u1.LastWriteTime, u2.LastWriteTime)) != 0)
  if((n = CompareFileTime(&u1.LastWriteTime, &u2.LastWriteTime)) != 0)
  
    return n;
  if (!u1.IsDirectory())
  {
    if((n = MyCompare(u1.Size, u2.Size)) != 0)
      return n;
  }
  return 0;
}

HRESULT UpdateSolidStd(NArchive::N7z::COutArchive &anArchive, 
    IInStream *anInStream,
    const CCompressionMethodMode *aMethod, 
    const CCompressionMethodMode *aHeaderMethod, 
    const NArchive::N7z::CArchiveDatabaseEx &aDatabase,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,
    IUpdateCallBack *anUpdateCallBack)
{
  if (aCopyIndexes.Size() != 0)
    return E_FAIL;

  if (anInStream != NULL)
    return E_FAIL;

  RETURN_IF_NOT_S_OK(anArchive.SkeepPrefixArchiveHeader());

  UINT64 aComplexity = 0;
  UINT32 aCompressIndex = 0, aCopyIndexIndex = 0;
  
  CArchiveDatabase aNewDatabase;

  bool aThereIsPackStream = false;
  for(int i = 0; i < aCompressStatuses.Size(); i++)
  {
    if (aCompressStatuses[i])
    {
      const CUpdateItemInfo &anUpdateItem = anUpdateItems[aCompressIndex++];
      aComplexity += anUpdateItem.Size;
      if (anUpdateItem.Commented)
        aComplexity += anUpdateItem.CommentRange.Size;
    }
    else
    {
      return E_FAIL;
    }
  }

  RETURN_IF_NOT_S_OK(anUpdateCallBack->SetTotal(aComplexity));
  UINT64 aCurrentComplexity = 0;
  RETURN_IF_NOT_S_OK(anUpdateCallBack->SetCompleted(&aCurrentComplexity));


  int aNumFiles = anUpdateItems.Size();

  CRecordVector<CRefItem> aRefItems;
  aRefItems.Reserve(aNumFiles);
  for (i = 0; i < aNumFiles; i++)
  {
    const CUpdateItemInfo &anUpdateItem = anUpdateItems[i];
    CRefItem aRefItem;
    aRefItem.Index = i;
    aRefItem.Data = &anUpdateItem;
    int aSlash1Pos = anUpdateItem.Name.ReverseFind(L'\\');
    int aSlash2Pos = anUpdateItem.Name.ReverseFind(L'/');
    int aSlashPos = MyMax(aSlash1Pos, aSlash2Pos);
    if (aSlashPos >= 0)
      aRefItem.NamePos = aSlashPos + 1;
    else
      aRefItem.NamePos = 0;
    int aDotPos = anUpdateItem.Name.ReverseFind(L'.');
    if (aDotPos < 0 || (aDotPos < aSlashPos && aSlashPos >= 0))
      aRefItem.ExtensionPos = anUpdateItem.Name.Length();
    else 
      aRefItem.ExtensionPos = aDotPos + 1;
    aRefItems.Add(aRefItem);
  }
  
  qsort(&aRefItems.Front(), aRefItems.Size(), sizeof(aRefItems[0]), CompareUpdateItems);

  CRecordVector<UINT32> anIndexes;
  anIndexes.Reserve(aNumFiles);
  for (i = 0; i < aNumFiles; i++)
  {
    UINT32 anIndex = aRefItems[i].Index;
    anIndexes.Add(anIndex);
    const CUpdateItemInfo &anUpdateItem = anUpdateItems[anIndex];
    CFileItemInfo aFileItem;
    // ConvertUnicodeToUTF(NItemName::MakeLegalName(anUpdateItem.Name), aFileItem.Name); // test it
    aFileItem.Name = NItemName::MakeLegalName(anUpdateItem.Name);

    // aFileItem.SetCreationTime(anOperation.ItemInfo.CreationTime);
    aFileItem.SetLastWriteTime(anUpdateItem.LastWriteTime);
    aFileItem.SetAttributes(anUpdateItem.Attributes);
    aFileItem.UnPackSize = anUpdateItem.Size;
    aFileItem.IsDirectory = anUpdateItem.IsDirectory();
    if (!aFileItem.IsDirectory && anUpdateItem.Size != 0)
      aThereIsPackStream = true;
    aNewDatabase.m_Files.Add(aFileItem);
  }

  if (aThereIsPackStream)
  {
    CComObjectNoLock<CFolderInStream> *anInStreamSpec = 
      new CComObjectNoLock<CFolderInStream>;
    CComPtr<ISequentialInStream> aSolidInStream(anInStreamSpec);

    anInStreamSpec->Init(anUpdateCallBack, &anIndexes.Front(), aNumFiles);
    
    CEncoder anEncoder(aMethod);
    CFolderItemInfo aFolderItem;

    CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = 
      new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> aLocalProgress = aLocalProgressSpec;
    aLocalProgressSpec->Init(anUpdateCallBack, true);
    CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
    aLocalCompressProgressSpec->Init(aLocalProgress, &aCurrentComplexity, NULL);

    RETURN_IF_NOT_S_OK(anEncoder.Encode(aSolidInStream, aFolderItem, 
        anArchive.m_Stream, aNewDatabase.m_PackSizes, aCompressProgress));

    // aFolderItem.NumFiles = aNumFiles;
    aNewDatabase.m_Folders.Add(aFolderItem);
    UINT32 aNumUnPackStreams = 0;
    for (i = 0; i < aNumFiles; i++)
    {
      CFileItemInfo &aFileItem = aNewDatabase.m_Files[i];
      
      aFileItem.FileCRC = anInStreamSpec->m_CRCs[i];
      aFileItem.FileCRCIsDefined = true;

      aFileItem.UnPackSize = anInStreamSpec->m_Sizes[i];
      if (!aFileItem.IsDirectory && aFileItem.UnPackSize != 0)
        aNumUnPackStreams++;
    }
    aNewDatabase.m_NumUnPackStreamsVector.Reserve(1);
    aNewDatabase.m_NumUnPackStreamsVector.Add(aNumUnPackStreams);
  }

  return anArchive.WriteDatabase(aNewDatabase, aHeaderMethod);
}

}}
