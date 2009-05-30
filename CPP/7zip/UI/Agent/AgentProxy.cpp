// AgentProxy.cpp

#include "StdAfx.h"

#include "../../../../C/Sort.h"

#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

#include "../Common/OpenArchive.h"

#include "AgentProxy.h"

using namespace NWindows;

int CProxyFolder::FindDirSubItemIndex(const UString &name, int &insertPos) const
{
  int left = 0, right = Folders.Size();
  for (;;)
  {
    if (left == right)
    {
      insertPos = left;
      return -1;
    }
    int mid = (left + right) / 2;
    int compare = name.CompareNoCase(Folders[mid].Name);
    if (compare == 0)
      return mid;
    if (compare < 0)
      right = mid;
    else
      left = mid + 1;
  }
}

int CProxyFolder::FindDirSubItemIndex(const UString &name) const
{
  int insertPos;
  return FindDirSubItemIndex(name, insertPos);
}

void CProxyFolder::AddFileSubItem(UInt32 index, const UString &name)
{
  Files.Add(CProxyFile());
  Files.Back().Name = name;
  Files.Back().Index = index;
}

CProxyFolder* CProxyFolder::AddDirSubItem(UInt32 index, bool leaf, const UString &name)
{
  int insertPos;
  int folderIndex = FindDirSubItemIndex(name, insertPos);
  if (folderIndex >= 0)
  {
    CProxyFolder *item = &Folders[folderIndex];
    if (leaf)
    {
      item->Index = index;
      item->IsLeaf = true;
    }
    return item;
  }
  Folders.Insert(insertPos, CProxyFolder());
  CProxyFolder *item = &Folders[insertPos];
  item->Name = name;
  item->Index = index;
  item->Parent = this;
  item->IsLeaf = leaf;
  return item;
}

void CProxyFolder::Clear()
{
  Folders.Clear();
  Files.Clear();
}

void CProxyFolder::GetPathParts(UStringVector &pathParts) const
{
  pathParts.Clear();
  UString result;
  const CProxyFolder *current = this;
  while (current->Parent != NULL)
  {
    pathParts.Insert(0, (const wchar_t *)current->Name);
    current = current->Parent;
  }
}

UString CProxyFolder::GetFullPathPrefix() const
{
  UString result;
  const CProxyFolder *current = this;
  while (current->Parent != NULL)
  {
    result = current->Name + UString(WCHAR_PATH_SEPARATOR) + result;
    current = current->Parent;
  }
  return result;
}

UString CProxyFolder::GetItemName(UInt32 index) const
{
  if (index < (UInt32)Folders.Size())
    return Folders[index].Name;
  return Files[index - Folders.Size()].Name;
}

void CProxyFolder::AddRealIndices(CUIntVector &realIndices) const
{
  if (IsLeaf)
    realIndices.Add(Index);
  int i;
  for (i = 0; i < Folders.Size(); i++)
    Folders[i].AddRealIndices(realIndices);
  for (i = 0; i < Files.Size(); i++)
    realIndices.Add(Files[i].Index);
}

void CProxyFolder::GetRealIndices(const UInt32 *indices, UInt32 numItems, CUIntVector &realIndices) const
{
  realIndices.Clear();
  for (UInt32 i = 0; i < numItems; i++)
  {
    int index = indices[i];
    int numDirItems = Folders.Size();
    if (index < numDirItems)
      Folders[index].AddRealIndices(realIndices);
    else
      realIndices.Add(Files[index - numDirItems].Index);
  }
  HeapSort(&realIndices.Front(), realIndices.Size());
}

///////////////////////////////////////////////
// CProxyArchive

static UInt64 GetSize(IInArchive *archive, UInt32 index, PROPID propID)
{
  NCOM::CPropVariant prop;
  if (archive->GetProperty(index, propID, &prop) == S_OK)
    if (prop.vt != VT_EMPTY)
      return ConvertPropVariantToUInt64(prop);
  return 0;
}

void CProxyFolder::CalculateSizes(IInArchive *archive)
{
  Size = PackSize = 0;
  NumSubFolders = Folders.Size();
  NumSubFiles = Files.Size();
  CrcIsDefined = true;
  Crc = 0;
  int i;
  for (i = 0; i < Files.Size(); i++)
  {
    UInt32 index = Files[i].Index;
    Size += GetSize(archive, index, kpidSize);
    PackSize += GetSize(archive, index, kpidPackSize);
    {
      NCOM::CPropVariant prop;
      if (archive->GetProperty(index, kpidCRC, &prop) == S_OK && prop.vt == VT_UI4)
        Crc += prop.ulVal;
      else
        CrcIsDefined = false;
    }
  }
  for (i = 0; i < Folders.Size(); i++)
  {
    CProxyFolder &f = Folders[i];
    f.CalculateSizes(archive);
    Size += f.Size;
    PackSize += f.PackSize;
    NumSubFiles += f.NumSubFiles;
    NumSubFolders += f.NumSubFolders;
    Crc += f.Crc;
    if (!f.CrcIsDefined)
      CrcIsDefined = false;
  }
}

HRESULT CProxyArchive::Load(const CArc &arc, IProgress *progress)
{
  RootFolder.Clear();
  IInArchive *archive = arc.Archive;
  {
    ThereIsPathProp = false;
    UInt32 numProps;
    archive->GetNumberOfProperties(&numProps);
    for (UInt32 i = 0; i < numProps; i++)
    {
      CMyComBSTR name;
      PROPID propID;
      VARTYPE varType;
      RINOK(archive->GetPropertyInfo(i, &name, &propID, &varType));
      if (propID == kpidPath)
      {
        ThereIsPathProp = true;
        break;
      }
    }
  }

  UInt32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));
  if (progress != NULL)
  {
    UInt64 totalItems = numItems;
    RINOK(progress->SetTotal(totalItems));
  }
  UString fileName;
  for (UInt32 i = 0; i < numItems; i++)
  {
    if (progress != NULL && (i & 0xFFFFF) == 0)
    {
      UInt64 currentItemIndex = i;
      RINOK(progress->SetCompleted(&currentItemIndex));
    }
    UString filePath;
    RINOK(arc.GetItemPath(i, filePath));
    CProxyFolder *curItem = &RootFolder;
    int len = filePath.Length();
    fileName.Empty();
    for (int j = 0; j < len; j++)
    {
      wchar_t c = filePath[j];
      if (c == WCHAR_PATH_SEPARATOR || c == L'/')
      {
        curItem = curItem->AddDirSubItem((UInt32)(Int32)-1, false, fileName);
        fileName.Empty();
      }
      else
        fileName += c;
    }

    bool isFolder;
    RINOK(IsArchiveItemFolder(archive, i, isFolder));
    if (isFolder)
      curItem->AddDirSubItem(i, true, fileName);
    else
      curItem->AddFileSubItem(i, fileName);
  }
  RootFolder.CalculateSizes(archive);
  return S_OK;
}
