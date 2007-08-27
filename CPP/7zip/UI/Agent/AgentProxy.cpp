// AgentProxy.cpp

#include "StdAfx.h"

#include "AgentProxy.h"

#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/Defs.h"
#include "../Common/OpenArchive.h"

extern "C"
{
  #include "../../../../C/Sort.h"
}

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
    if(leaf)
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
    result = current->Name + UString(L'\\') + result;
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
  for(i = 0; i < Folders.Size(); i++)
    Folders[i].AddRealIndices(realIndices);
  for(i = 0; i < Files.Size(); i++)
    realIndices.Add(Files[i].Index);
}

void CProxyFolder::GetRealIndices(const UInt32 *indices, UInt32 numItems, CUIntVector &realIndices) const
{
  realIndices.Clear();
  for(UInt32 i = 0; i < numItems; i++)
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

HRESULT CProxyArchive::Reload(IInArchive *archive, IProgress *progress)
{
  RootFolder.Clear();
  return ReadObjects(archive, progress);
}

HRESULT CProxyArchive::Load(IInArchive *archive, 
    const UString &defaultName, 
    // const FILETIME &defaultTime,
    // UInt32 defaultAttributes,
    IProgress *progress)
{
  DefaultName = defaultName;
  // DefaultTime = defaultTime;
  // DefaultAttributes = defaultAttributes;
  return Reload(archive, progress);
}

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
    PackSize += GetSize(archive, index, kpidPackedSize);
    {
      NCOM::CPropVariant prop;
      if (archive->GetProperty(index, kpidCRC, &prop) == S_OK)
      {
        if (prop.vt == VT_UI4)
          Crc += prop.ulVal;
        else
          CrcIsDefined = false;
      }
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

HRESULT CProxyArchive::ReadObjects(IInArchive *archive, IProgress *progress)
{
  UInt32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));
  if (progress != NULL)
  {
    UINT64 totalItems = numItems; 
    RINOK(progress->SetTotal(totalItems));
  }
  for(UInt32 i = 0; i < numItems; i++)
  {
    if (progress != NULL)
    {
      UINT64 currentItemIndex = i; 
      RINOK(progress->SetCompleted(&currentItemIndex));
    }
    NCOM::CPropVariant propVariantPath;
    RINOK(archive->GetProperty(i, kpidPath, &propVariantPath));
    CProxyFolder *currentItem = &RootFolder;
    UString fileName;
    if(propVariantPath.vt == VT_EMPTY)
      fileName = DefaultName;
    else
    {
      if(propVariantPath.vt != VT_BSTR)
        return E_FAIL;
      UString filePath = propVariantPath.bstrVal;

      int len = filePath.Length();
      for (int i = 0; i < len; i++)
      {
        wchar_t c = filePath[i];
        if (c == '\\' || c == '/')
        {
          currentItem = currentItem->AddDirSubItem((UInt32)(Int32)-1, false, fileName);
          fileName.Empty();
        }
        else
          fileName += c;
      }
    }

    NCOM::CPropVariant propVariantIsFolder;
    bool isFolder;
    RINOK(IsArchiveItemFolder(archive, i, isFolder));
    if(isFolder)
      currentItem->AddDirSubItem(i, true, fileName);
    else
      currentItem->AddFileSubItem(i, fileName);
  }
  RootFolder.CalculateSizes(archive);
  return S_OK;
}

