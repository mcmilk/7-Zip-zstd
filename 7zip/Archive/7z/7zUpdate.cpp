// UpdateMain.cpp

#include "StdAfx.h"

#include "7zUpdate.h"
#include "7zFolderInStream.h"
#include "7zEncode.h"
#include "7zHandler.h"
#include "7zOut.h"

#ifndef EXCLUDE_COM
#include "7zMethods.h"
#endif

#include "../../Compress/Copy/CopyCoder.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/LimitedStreams.h"
#include "../Common/ItemNameUtils.h"

namespace NArchive {
namespace N7z {

static const wchar_t *kMatchFinderForBCJ2_LZMA = L"BT2";
static const UINT32 kDictionaryForBCJ2_LZMA = 1 << 20;
const UINT32 kAlgorithmForBCJ2_LZMA = 2;
const UINT32 kNumFastBytesForBCJ2_LZMA = 64;

static HRESULT CopyBlock(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, progress);
}

static HRESULT WriteRange(IInStream *inStream, 
    ISequentialOutStream *outStream, 
    const CUpdateRange &range, 
    IProgress *progress,
    UINT64 &currentComplexity)
{
  UINT64 position;
  inStream->Seek(range.Position, STREAM_SEEK_SET, &position);

  CLimitedSequentialInStream *streamSpec = new 
      CLimitedSequentialInStream;
  CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
  streamSpec->Init(inStream, range.Size);

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(progress, true);
  
  CLocalCompressProgressInfo *localCompressProgressSpec = 
      new CLocalCompressProgressInfo;
  CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

  localCompressProgressSpec->Init(localProgress, &currentComplexity, &currentComplexity);

  HRESULT result = CopyBlock(inStreamLimited, outStream, compressProgress);
  currentComplexity += range.Size;
  return result;
}

int CUpdateItem::GetExtensionPos() const
{
  int slash1Pos = Name.ReverseFind(L'\\');
  int slash2Pos = Name.ReverseFind(L'/');
  int slashPos = MyMax(slash1Pos, slash2Pos);
  int dotPos = Name.ReverseFind(L'.');
  if (dotPos < 0 || (dotPos < slashPos && slashPos >= 0))
    return Name.Length();
  return dotPos + 1;
}

UString CUpdateItem::GetExtension() const
{
  return Name.Mid(GetExtensionPos());
}

struct CFolderRef
{
  const CArchiveDatabaseEx *Database;
  int FolderIndex;
};

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareMethodIDs(const CMethodID &a1, const CMethodID &a2)
{
  for (int i = 0; i < a1.IDSize && i < a2.IDSize; i++)
    RINOZ(MyCompare(a1.ID[i], a2.ID[i]));
  return MyCompare(a1.IDSize, a2.IDSize);
}

static int CompareBuffers(const CByteBuffer &a1, const CByteBuffer &a2)
{
  int c1 = a1.GetCapacity();
  int c2 = a2.GetCapacity();
  RINOZ(MyCompare(c1, c2));
  for (int i = 0; i < c1; i++)
    RINOZ(MyCompare(a1[i], a2[i]));
  return 0;
}

static int CompareAltCoders(const CAltCoderInfo &a1, const CAltCoderInfo &a2)
{
  RINOZ(CompareMethodIDs(a1.MethodID, a2.MethodID));
  return CompareBuffers(a1.Properties, a2.Properties);
}

static int CompareCoders(const CCoderInfo &c1, const CCoderInfo &c2)
{
  RINOZ(MyCompare(c1.NumInStreams, c2.NumInStreams));
  RINOZ(MyCompare(c1.NumOutStreams, c2.NumOutStreams));
  int s1 = c1.AltCoders.Size();
  int s2 = c2.AltCoders.Size();
  RINOZ(MyCompare(s1, s2));
  for (int i = 0; i < s1; i++)
    RINOZ(CompareAltCoders(c1.AltCoders[i], c2.AltCoders[i]));
  return 0;
}

static int CompareBindPairs(const CBindPair &b1, const CBindPair &b2)
{
  RINOZ(MyCompare(b1.InIndex, b2.InIndex));
  return MyCompare(b1.OutIndex, b2.OutIndex);
}

static int CompareFolders(const CFolder &f1, const CFolder &f2)
{
  int s1 = f1.Coders.Size();
  int s2 = f2.Coders.Size();
  RINOZ(MyCompare(s1, s2));
  int i;
  for (i = 0; i < s1; i++)
    RINOZ(CompareCoders(f1.Coders[i], f2.Coders[i]));
  s1 = f1.BindPairs.Size();
  s2 = f2.BindPairs.Size();
  RINOZ(MyCompare(s1, s2));
  for (i = 0; i < s1; i++)
    RINOZ(CompareBindPairs(f1.BindPairs[i], f2.BindPairs[i]));
  return 0;
}

static int CompareFiles(const CFileItem &f1, const CFileItem &f2)
{
  return MyStringCollateNoCase(f1.Name, f2.Name);
}

static int __cdecl CompareFolderRefs(const void *p1, const void *p2)
{
  const CFolderRef &a1 = *((const CFolderRef *)p1);
  const CFolderRef &a2 = *((const CFolderRef *)p2);
  const CArchiveDatabaseEx &d1 = *a1.Database;
  const CArchiveDatabaseEx &d2 = *a2.Database;
  RINOZ(CompareFolders(
      d1.Folders[a1.FolderIndex],
      d2.Folders[a2.FolderIndex]));
  RINOZ(MyCompare(
      d1.NumUnPackStreamsVector[a1.FolderIndex],
      d2.NumUnPackStreamsVector[a2.FolderIndex]));
  if (d1.NumUnPackStreamsVector[a1.FolderIndex] == 0)
    return 0;
  return CompareFiles(
      d1.Files[d1.FolderStartFileIndex[a1.FolderIndex]],
      d2.Files[d2.FolderStartFileIndex[a2.FolderIndex]]);
}

////////////////////////////////////////////////////////////

static int __cdecl CompareEmptyItems(const void *p1, const void *p2)
{
  const CUpdateItem &u1 = **((CUpdateItem **)p1);
  const CUpdateItem &u2 = **((CUpdateItem **)p2);
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
    int n = MyStringCollateNoCase(u1.Name, u2.Name);
    return (u1.IsAnti ? (-n) : n);
  }
  if (u1.IsAnti != u2.IsAnti)
    return (u1.IsAnti ? 1 : -1);
  return MyStringCollateNoCase(u1.Name, u2.Name);
}

struct CRefItem
{
  UINT32 Index;
  const CUpdateItem *UpdateItem;
  UINT32 ExtensionPos;
  UINT32 NamePos;
  bool SortByType;
  CRefItem(UINT32 index, const CUpdateItem &updateItem, bool sortByType):
    SortByType(sortByType),
    Index(index),
    UpdateItem(&updateItem),
    ExtensionPos(0),
    NamePos(0)
  {
    if (sortByType)
    {
      int slash1Pos = updateItem.Name.ReverseFind(L'\\');
      int slash2Pos = updateItem.Name.ReverseFind(L'/');
      int slashPos = MyMax(slash1Pos, slash2Pos);
      if (slashPos >= 0)
        NamePos = slashPos + 1;
      else
        NamePos = 0;
      int dotPos = updateItem.Name.ReverseFind(L'.');
      if (dotPos < 0 || (dotPos < slashPos && slashPos >= 0))
        ExtensionPos = updateItem.Name.Length();
      else 
        ExtensionPos = dotPos + 1;
    }
  }
};

static int __cdecl CompareUpdateItems(const void *p1, const void *p2)
{
  const CRefItem &a1 = *((CRefItem *)p1);
  const CRefItem &a2 = *((CRefItem *)p2);
  const CUpdateItem &u1 = *a1.UpdateItem;
  const CUpdateItem &u2 = *a2.UpdateItem;
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
    n = MyStringCollateNoCase(u1.Name, u2.Name);
    return (u1.IsAnti ? (-n) : n);
  }
  if (a1.SortByType)
  {
    RINOZ(MyStringCollateNoCase(u1.Name + a1.ExtensionPos, u2.Name + a2.ExtensionPos));
    RINOZ(MyStringCollateNoCase(u1.Name + a1.NamePos, u2.Name + a2.NamePos));
    if (u1.LastWriteTimeIsDefined && u2.LastWriteTimeIsDefined)
      RINOZ(CompareFileTime(&u1.LastWriteTime, &u2.LastWriteTime));
    RINOZ(MyCompare(u1.Size, u2.Size))
  }
  return MyStringCollateNoCase(u1.Name, u2.Name);
}

struct CSolidGroup
{
  CCompressionMethodMode Method;
  CRecordVector<UINT32> Indices;
};

static wchar_t *g_ExeExts[] =
{
  L"dll",
  L"exe",
  L"ocx",
  L"sfx",
  L"sys"
};

static bool IsExeFile(const UString &ext)
{
  for (int i = 0; i < sizeof(g_ExeExts) / sizeof(g_ExeExts[0]); i++)
    if (ext.CompareNoCase(g_ExeExts[i]) == 0)
      return true;
  return false;
}

static CMethodID k_BCJ_X86 = { { 0x3, 0x3, 0x1, 0x3 }, 4 };
static CMethodID k_BCJ2 = { { 0x3, 0x3, 0x1, 0x1B }, 4 };
static CMethodID k_LZMA = { { 0x3, 0x1, 0x1 }, 3 };

static bool GetMethodFull(const CMethodID &methodID, 
    UINT32 numInStreams, CMethodFull &methodResult)
{
  methodResult.MethodID = methodID;
  methodResult.NumInStreams = numInStreams;
  methodResult.NumOutStreams = 1;

  #ifndef EXCLUDE_COM
  CMethodInfo methodInfo;
  if (!GetMethodInfo(methodID, methodInfo))
    return false;
  if (!methodInfo.EncoderIsAssigned)
    return false;
  methodResult.EncoderClassID = methodInfo.Encoder;
  methodResult.FilePath = methodInfo.FilePath;
  if (methodInfo.NumOutStreams != 1 || methodInfo.NumInStreams != numInStreams)
    return false;
  #endif
  return true;
}

static bool MakeExeMethod(const CCompressionMethodMode &method, 
    bool bcj2Filter, CCompressionMethodMode &exeMethod)
{
  exeMethod = method;
  if (bcj2Filter)
  {
    CMethodFull methodFull;
    if (!GetMethodFull(k_BCJ2, 4, methodFull))
      return false;
    exeMethod.Methods.Insert(0, methodFull);
    if (!GetMethodFull(k_LZMA, 1, methodFull))
      return false;
    {
      CProperty property;
      property.PropID = NCoderPropID::kAlgorithm;
      property.Value = kAlgorithmForBCJ2_LZMA;
      methodFull.CoderProperties.Add(property);
    }
    {
      CProperty property;
      property.PropID = NCoderPropID::kMatchFinder;
      property.Value = kMatchFinderForBCJ2_LZMA;
      methodFull.CoderProperties.Add(property);
    }
    {
      CProperty property;
      property.PropID = NCoderPropID::kDictionarySize;
      property.Value = kDictionaryForBCJ2_LZMA;
      methodFull.CoderProperties.Add(property);
    }
    {
      CProperty property;
      property.PropID = NCoderPropID::kNumFastBytes;
      property.Value = kNumFastBytesForBCJ2_LZMA;
      methodFull.CoderProperties.Add(property);
    }

    exeMethod.Methods.Add(methodFull);
    exeMethod.Methods.Add(methodFull);
    CBind bind;

    bind.OutCoder = 0;
    bind.InStream = 0;

    bind.InCoder = 1;
    bind.OutStream = 0;
    exeMethod.Binds.Add(bind);

    bind.InCoder = 2;
    bind.OutStream = 1;
    exeMethod.Binds.Add(bind);

    bind.InCoder = 3;
    bind.OutStream = 2;
    exeMethod.Binds.Add(bind);
  }
  else
  {
    CMethodFull methodFull;
    if (!GetMethodFull(k_BCJ_X86, 1, methodFull))
      return false;
    exeMethod.Methods.Insert(0, methodFull);
    CBind bind;
    bind.OutCoder = 0;
    bind.InStream = 0;
    bind.InCoder = 1;
    bind.OutStream = 0;
    exeMethod.Binds.Add(bind);
  }
  return true;
}   

static void SplitFilesToGroups(
    const CCompressionMethodMode &method, 
    bool useFilters, bool maxFilter,
    const CObjectVector<CUpdateItem> &updateItems,
    CObjectVector<CSolidGroup> &groups)
{
  if (method.Methods.Size() != 1 || method.Binds.Size() != 0)
    useFilters = false;
  groups.Clear();
  groups.Add(CSolidGroup());
  groups.Add(CSolidGroup());
  CSolidGroup &generalGroup = groups[0];
  CSolidGroup &exeGroup = groups[1];
  generalGroup.Method = method;
  int i;
  for (i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &updateItem = updateItems[i];
    if (!updateItem.NewData)
      continue;
    if (!updateItem.HasStream())
      continue;
    if (useFilters)
    {
      const UString name = updateItem.Name;
      int dotPos = name.ReverseFind(L'.');
      if (dotPos >= 0)
      {
        UString ext = name.Mid(dotPos + 1);
        if (IsExeFile(ext))
        {
          exeGroup.Indices.Add(i);
          continue;
        }
      }
    }
    generalGroup.Indices.Add(i);
  }
  if (exeGroup.Indices.Size() > 0)
    if (!MakeExeMethod(method, maxFilter, exeGroup.Method))
      exeGroup.Method = method;
  for (i = 0; i < groups.Size();)
    if (groups[i].Indices.Size() == 0)
      groups.Delete(i);
    else
      i++;
}

static void FromUpdateItemToFileItem(const CUpdateItem &updateItem, 
    CFileItem &file)
{
  file.Name = NItemName::MakeLegalName(updateItem.Name);
  if (updateItem.AttributesAreDefined)
    file.SetAttributes(updateItem.Attributes);
  
  // if (updateItem.CreationTimeIsDefined)
  //   file.SetCreationTime(updateItem.ItemInfo.CreationTime);
  
  if (updateItem.LastWriteTimeIsDefined)
    file.SetLastWriteTime(updateItem.LastWriteTime);
  
  file.UnPackSize = updateItem.Size;
  file.IsDirectory = updateItem.IsDirectory;
  file.IsAnti = updateItem.IsAnti;
  file.HasStream = updateItem.HasStream();
}

HRESULT Update(const NArchive::N7z::CArchiveDatabaseEx &database,
    CObjectVector<CUpdateItem> &updateItems,
    IOutStream *outStream,
    IInStream *inStream,
    NArchive::N7z::CInArchiveInfo *inArchiveInfo,
    const CCompressionMethodMode &method,
    const CCompressionMethodMode *headerMethod,
    bool useFilters,
    bool maxFilter,
    bool useAdditionalHeaderStreams, 
    bool compressMainHeader,
    IArchiveUpdateCallback *updateCallback,
    UINT64 numSolidFiles, UINT64 numSolidBytes, bool solidExtension,
    bool removeSfxBlock)
{
  if (numSolidFiles == 0)
    numSolidFiles = 1;

  UINT64 startBlockSize = inArchiveInfo != 0 ? 
      inArchiveInfo->StartPosition: 0;
  if (startBlockSize > 0 && !removeSfxBlock)
  {
    CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
    CMyComPtr<ISequentialInStream> limitedStream(streamSpec);
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    streamSpec->Init(inStream, startBlockSize);
    RINOK(CopyBlock(limitedStream, outStream, NULL));
  }

  CRecordVector<int> fileIndexToUpdateIndexMap;
  fileIndexToUpdateIndexMap.Reserve(database.Files.Size());
  int i;
  for (i = 0; i < database.Files.Size(); i++)
    fileIndexToUpdateIndexMap.Add(-1);
  for(i = 0; i < updateItems.Size(); i++)
  {
    int index = updateItems[i].IndexInArchive;
    if (index != -1)
      fileIndexToUpdateIndexMap[index] = i;
  }

  CRecordVector<CFolderRef> folderRefs;
  for(i = 0; i < database.Folders.Size(); i++)
  {
    UINT64 indexInFolder = 0;
    UINT64 numCopyItems = 0;
    UINT64 numUnPackStreams = database.NumUnPackStreamsVector[i];
    for (int fileIndex = database.FolderStartFileIndex[i];
        indexInFolder < numUnPackStreams; fileIndex++)
    {
      if (database.Files[fileIndex].HasStream)
      {
        indexInFolder++;
        int updateIndex = fileIndexToUpdateIndexMap[fileIndex];
        if (updateIndex >= 0)
          if (!updateItems[updateIndex].NewData)
            numCopyItems++;
      }
    }
    if (numCopyItems != numUnPackStreams && numCopyItems != 0)
      return E_NOTIMPL; // It needs repacking !!!
    if (numCopyItems > 0)
    {
      CFolderRef folderRef;
      folderRef.Database = &database;
      folderRef.FolderIndex = i;
      folderRefs.Add(folderRef);
    }
  }
  qsort(&folderRefs.Front(), folderRefs.Size(), sizeof(folderRefs[0]), 
      CompareFolderRefs);

  CArchiveDatabase newDatabase;

  /////////////////////////////////////////
  // Write Empty Files & Folders

  CRecordVector<const CUpdateItem *> emptyRefs;
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &updateItem = updateItems[i];
    if (updateItem.NewData)
    {
      if (updateItem.HasStream())
        continue;
    }
    else
      if (updateItem.IndexInArchive != -1)
        if (database.Files[updateItem.IndexInArchive].HasStream)
          continue;
    emptyRefs.Add(&updateItem);
  }
  qsort(&emptyRefs.Front(), emptyRefs.Size(), sizeof(emptyRefs[0]), 
      CompareEmptyItems);
  for(i = 0; i < emptyRefs.Size(); i++)
  {
    const CUpdateItem &updateItem = *emptyRefs[i];
    CFileItem file;
    if (updateItem.NewProperties)
      FromUpdateItemToFileItem(updateItem, file);
    else
      file = database.Files[updateItem.IndexInArchive];
    newDatabase.Files.Add(file);
  }

  ////////////////////////////

  COutArchive archive;
  archive.Create(outStream);
  RINOK(archive.SkeepPrefixArchiveHeader());
  UINT64 complexity = 0;
  for(i = 0; i < folderRefs.Size(); i++)
    complexity += database.GetFolderFullPackSize(folderRefs[i].FolderIndex);
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &updateItem = updateItems[i];
    if (updateItem.NewData)
      complexity += updateItem.Size;
  }
  RINOK(updateCallback->SetTotal(complexity));
  complexity = 0;
  RINOK(updateCallback->SetCompleted(&complexity));

  /////////////////////////////////////////
  // Write Copy Items

  for(i = 0; i < folderRefs.Size(); i++)
  {
    int folderIndex = folderRefs[i].FolderIndex;
    
    RINOK(WriteRange(inStream, archive.Stream,
        CUpdateRange(database.GetFolderStreamPos(folderIndex, 0),
        database.GetFolderFullPackSize(folderIndex)),
        updateCallback, complexity));
    
    const CFolder &folder = database.Folders[folderIndex];
    UINT32 startIndex = database.FolderStartPackStreamIndex[folderIndex];
    int j;
    for (j = 0; j < folder.PackStreams.Size(); j++)
    {
      newDatabase.PackSizes.Add(database.PackSizes[startIndex + j]);
      // newDatabase.PackCRCsDefined.Add(database.PackCRCsDefined[startIndex + j]);
      // newDatabase.PackCRCs.Add(database.PackCRCs[startIndex + j]);
    }
    newDatabase.Folders.Add(folder);

    UINT64 numUnPackStreams = database.NumUnPackStreamsVector[folderIndex];
    newDatabase.NumUnPackStreamsVector.Add(numUnPackStreams);

    UINT64 indexInFolder = 0;
    for (j = database.FolderStartFileIndex[folderIndex];
        indexInFolder < numUnPackStreams; j++)
    {
      CFileItem file = database.Files[j];
      if (file.HasStream)
      {
        indexInFolder++;
        int updateIndex = fileIndexToUpdateIndexMap[j];
        if (updateIndex >= 0)
        {
          const CUpdateItem &updateItem = updateItems[updateIndex];
          if (updateItem.NewProperties)
          {
            CFileItem file2;
            FromUpdateItemToFileItem(updateItem, file2);
            file2.UnPackSize = file.UnPackSize;
            file2.FileCRC = file.FileCRC;
            file2.FileCRCIsDefined = file.FileCRCIsDefined;
            file2.HasStream = file.HasStream;
            file = file2;
          }
        }
        newDatabase.Files.Add(file);
      }
    }
  }

  /////////////////////////////////////////
  // Compress New Files

  CObjectVector<CSolidGroup> groups;
  SplitFilesToGroups(method, useFilters, maxFilter, updateItems, groups);

  for (int groupIndex = 0; groupIndex < groups.Size(); groupIndex++)
  {
    const CSolidGroup &group = groups[groupIndex];
    int numFiles = group.Indices.Size();
    if (numFiles == 0)
      continue;
    CRecordVector<CRefItem> refItems;
    refItems.Reserve(numFiles);
    for (i = 0; i < numFiles; i++)
      refItems.Add(CRefItem(group.Indices[i], 
          updateItems[group.Indices[i]], numSolidFiles > 1));
    qsort(&refItems.Front(), refItems.Size(), sizeof(refItems[0]), CompareUpdateItems);
    
    CRecordVector<UINT32> indices;
    indices.Reserve(numFiles);

    int startFileIndexInDatabase = newDatabase.Files.Size();
    for (i = 0; i < numFiles; i++)
    {
      UINT32 index = refItems[i].Index;
      indices.Add(index);
      const CUpdateItem &updateItem = updateItems[index];
      CFileItem file;
      if (updateItem.NewProperties)
        FromUpdateItemToFileItem(updateItem, file);
      else
        file = database.Files[updateItem.IndexInArchive];
      if (file.IsAnti || file.IsDirectory)
        return E_FAIL;
      newDatabase.Files.Add(file);
    }
    
    CEncoder encoder(group.Method);

    for (i = 0; i < numFiles;)
    {
      UINT64 totalSize = 0;
      int numSubFiles;
      UString prevExtension;
      for (numSubFiles = 0; i + numSubFiles < numFiles && 
          numSubFiles < numSolidFiles; numSubFiles++)
      {
        const CUpdateItem &updateItem = updateItems[indices[i + numSubFiles]];
        totalSize += updateItem.Size;
        if (totalSize > numSolidBytes)
          break;
        if (solidExtension)
        {
          UString ext = updateItem.GetExtension();
          if (numSubFiles == 0)
            prevExtension = ext;
          else
            if (ext.CollateNoCase(prevExtension) != 0)
              break;
        }
      }
      if (numSubFiles < 1)
        numSubFiles = 1;

      CFolderInStream *inStreamSpec = new CFolderInStream;
      CMyComPtr<ISequentialInStream> solidInStream(inStreamSpec);
      inStreamSpec->Init(updateCallback, &indices[i], numSubFiles);
      
      CFolder folderItem;
      CLocalProgress *localProgressSpec = new CLocalProgress;
      CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
      localProgressSpec->Init(updateCallback, true);
      CLocalCompressProgressInfo *localCompressProgressSpec = new CLocalCompressProgressInfo;
      CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
      localCompressProgressSpec->Init(localProgress, &complexity, NULL);
      
      RINOK(encoder.Encode(solidInStream, NULL, folderItem, 
        archive.Stream, newDatabase.PackSizes, compressProgress));
      // for()
      // newDatabase.PackCRCsDefined.Add(false);
      // newDatabase.PackCRCs.Add(0);
      
      newDatabase.Folders.Add(folderItem);
      
      UINT32 numUnPackStreams = 0;
      for (int subIndex = 0; subIndex < numSubFiles; subIndex++)
      {
        CFileItem &file = newDatabase.Files[
          startFileIndexInDatabase + i + subIndex];
        file.FileCRC = inStreamSpec->CRCs[subIndex];
        file.UnPackSize = inStreamSpec->Sizes[subIndex];
        if (file.UnPackSize != 0)
        {
          file.FileCRCIsDefined = true;
          file.HasStream = true;
          numUnPackStreams++;
          complexity += file.UnPackSize;
        }
        else
        {
          file.FileCRCIsDefined = false;
          file.HasStream = false;
        }
      }
      newDatabase.NumUnPackStreamsVector.Add(numUnPackStreams);
      i += numSubFiles;
    }
  }
  if (newDatabase.Files.Size() != updateItems.Size())
    return E_FAIL;

  return archive.WriteDatabase(newDatabase, headerMethod, 
      useAdditionalHeaderStreams, compressMainHeader);
}

}}