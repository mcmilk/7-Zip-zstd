// AgentProxy.cpp

#include "StdAfx.h"

#include "../../../../C/Sort.h"
#include "../../../../C/CpuArch.h"

#include "../../../Common/Wildcard.h"

#include "../../../Windows/PropVariant.h"
#include "../../../Windows/PropVariantConv.h"

#include "AgentProxy.h"

using namespace NWindows;

int CProxyArchive::FindDirSubItemIndex(unsigned folderIndex, const UString &name, unsigned &insertPos) const
{
  const CRecordVector<unsigned> &subFolders = Folders[folderIndex].Folders;
  unsigned left = 0, right = subFolders.Size();
  for (;;)
  {
    if (left == right)
    {
      insertPos = left;
      return -1;
    }
    unsigned mid = (left + right) / 2;
    unsigned folderIndex = subFolders[mid];
    int compare = CompareFileNames(name, Folders[folderIndex].Name);
    if (compare == 0)
      return folderIndex;
    if (compare < 0)
      right = mid;
    else
      left = mid + 1;
  }
}

int CProxyArchive::FindDirSubItemIndex(unsigned folderIndex, const UString &name) const
{
  unsigned insertPos;
  return FindDirSubItemIndex(folderIndex, name, insertPos);
}

void CProxyFolder::AddFileSubItem(UInt32 index, const UString &name)
{
  CProxyFile &f = Files.AddNew();
  f.Index = index;
  f.Name = name;
}

unsigned CProxyArchive::AddDirSubItem(unsigned folderIndex, UInt32 index, bool leaf, const UString &name)
{
  unsigned insertPos;
  int subFolderIndex = FindDirSubItemIndex(folderIndex, name, insertPos);
  if (subFolderIndex >= 0)
  {
    CProxyFolder &item = Folders[subFolderIndex];
    if (leaf)
    {
      item.Index = index;
      item.IsLeaf = true;
    }
    return subFolderIndex;
  }
  subFolderIndex = Folders.Size();
  Folders[folderIndex].Folders.Insert(insertPos, subFolderIndex);
  CProxyFolder &item = Folders.AddNew();
  item.Name = name;
  item.Index = index;
  item.Parent = folderIndex;
  item.IsLeaf = leaf;
  return subFolderIndex;
}

void CProxyFolder::Clear()
{
  Folders.Clear();
  Files.Clear();
}

void CProxyArchive::GetPathParts(int folderIndex, UStringVector &pathParts) const
{
  pathParts.Clear();
  while (folderIndex >= 0)
  {
    const CProxyFolder &folder = Folders[folderIndex];
    folderIndex = folder.Parent;
    if (folderIndex < 0)
      break;
    pathParts.Insert(0, folder.Name);
  }
}

UString CProxyArchive::GetFullPathPrefix(int folderIndex) const
{
  UString result;
  while (folderIndex >= 0)
  {
    const CProxyFolder &folder = Folders[folderIndex];
    folderIndex = folder.Parent;
    if (folderIndex < 0)
      break;
    result = folder.Name + UString(WCHAR_PATH_SEPARATOR) + result;
  }
  return result;
}

void CProxyArchive::AddRealIndices(unsigned folderIndex, CUIntVector &realIndices) const
{
  const CProxyFolder &folder = Folders[folderIndex];
  if (folder.IsLeaf)
    realIndices.Add(folder.Index);
  unsigned i;
  for (i = 0; i < folder.Folders.Size(); i++)
    AddRealIndices(folder.Folders[i], realIndices);
  for (i = 0; i < folder.Files.Size(); i++)
    realIndices.Add(folder.Files[i].Index);
}

int CProxyArchive::GetRealIndex(unsigned folderIndex, unsigned index) const
{
  const CProxyFolder &folder = Folders[folderIndex];
  unsigned numDirItems = folder.Folders.Size();
  if (index < numDirItems)
  {
    const CProxyFolder &f = Folders[folder.Folders[index]];
    if (f.IsLeaf)
      return f.Index;
    return -1;
  }
  return folder.Files[index - numDirItems].Index;
}

void CProxyArchive::GetRealIndices(unsigned folderIndex, const UInt32 *indices, UInt32 numItems, CUIntVector &realIndices) const
{
  const CProxyFolder &folder = Folders[folderIndex];
  realIndices.Clear();
  for (UInt32 i = 0; i < numItems; i++)
  {
    UInt32 index = indices[i];
    unsigned numDirItems = folder.Folders.Size();
    if (index < numDirItems)
      AddRealIndices(folder.Folders[index], realIndices);
    else
      realIndices.Add(folder.Files[index - numDirItems].Index);
  }
  HeapSort(&realIndices.Front(), realIndices.Size());
}

///////////////////////////////////////////////
// CProxyArchive

static bool GetSize(IInArchive *archive, UInt32 index, PROPID propID, UInt64 &size)
{
  size = 0;
  NCOM::CPropVariant prop;
  if (archive->GetProperty(index, propID, &prop) != S_OK)
    throw 20120228;
  return ConvertPropVariantToUInt64(prop, size);
}

void CProxyArchive::CalculateSizes(unsigned folderIndex, IInArchive *archive)
{
  CProxyFolder &folder = Folders[folderIndex];
  folder.Size = folder.PackSize = 0;
  folder.NumSubFolders = folder.Folders.Size();
  folder.NumSubFiles = folder.Files.Size();
  folder.CrcIsDefined = true;
  folder.Crc = 0;
  unsigned i;
  for (i = 0; i < folder.Files.Size(); i++)
  {
    UInt32 index = folder.Files[i].Index;
    UInt64 size, packSize;
    bool sizeDefined = GetSize(archive, index, kpidSize, size);
    folder.Size += size;
    GetSize(archive, index, kpidPackSize, packSize);
    folder.PackSize += packSize;
    {
      NCOM::CPropVariant prop;
      if (archive->GetProperty(index, kpidCRC, &prop) == S_OK)
      {
        if (prop.vt == VT_UI4)
          folder.Crc += prop.ulVal;
        else if (prop.vt != VT_EMPTY || size != 0 || !sizeDefined)
          folder.CrcIsDefined = false;
      }
      else
        folder.CrcIsDefined = false;
    }
  }
  for (i = 0; i < folder.Folders.Size(); i++)
  {
    unsigned subFolderIndex = folder.Folders[i];
    CProxyFolder &f = Folders[subFolderIndex];
    CalculateSizes(subFolderIndex, archive);
    folder.Size += f.Size;
    folder.PackSize += f.PackSize;
    folder.NumSubFiles += f.NumSubFiles;
    folder.NumSubFolders += f.NumSubFolders;
    folder.Crc += f.Crc;
    if (!f.CrcIsDefined)
      folder.CrcIsDefined = false;
  }
}

HRESULT CProxyArchive::Load(const CArc &arc, IProgress *progress)
{
  /*
  DWORD tickCount = GetTickCount();
  for (int ttt = 0; ttt < 1000; ttt++) {
  */

  Folders.Clear();
  Folders.AddNew();
  IInArchive *archive = arc.Archive;

  UInt32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));
  if (progress)
    RINOK(progress->SetTotal(numItems));
  UString filePath;
  UString fileName;
  for (UInt32 i = 0; i < numItems; i++)
  {
    if (progress && (i & 0xFFFFF) == 0)
    {
      UInt64 currentItemIndex = i;
      RINOK(progress->SetCompleted(&currentItemIndex));
    }
    RINOK(arc.GetItemPath(i, filePath));
    unsigned curItem = 0;
    unsigned len = filePath.Len();
    fileName.Empty();

    /*
    if (arc.Ask_Deleted)
    {
      bool isDeleted = false;
      RINOK(Archive_IsItem_Deleted(archive, i, isDeleted));
      if (isDeleted)
        curItem = AddDirSubItem(curItem, (UInt32)(Int32)-1, false, L"[DELETED]");
    }
    */

    for (unsigned j = 0; j < len; j++)
    {
      wchar_t c = filePath[j];
      if (c == WCHAR_PATH_SEPARATOR || c == L'/')
      {
        curItem = AddDirSubItem(curItem, (UInt32)(Int32)-1, false, fileName);
        fileName.Empty();
      }
      else
        fileName += c;
    }

    /*
    that code must be implemeted to hide alt streams in list.
    if (arc.Ask_AltStreams)
    {
      bool isAltStream;
      RINOK(Archive_IsItem_AltStream(archive, i, isAltStream));
      if (isAltStream)
      {

      }
    }
    */

    bool isFolder;
    RINOK(Archive_IsItem_Folder(archive, i, isFolder));
    if (isFolder)
      AddDirSubItem(curItem, i, true, fileName);
    else
      Folders[curItem].AddFileSubItem(i, fileName);
  }
  CalculateSizes(0, archive);

  /*
  }
  char s[128];
  sprintf(s, "load archive %7d ms", GetTickCount() - tickCount);
  OutputDebugStringA(s);
  */

  return S_OK;
}



// ---------- for Tree-mode archive ----------

void CProxyArchive2::GetPathParts(int folderIndex, UStringVector &pathParts) const
{
  pathParts.Clear();
  while (folderIndex > 0)
  {
    const CProxyFolder2 &folder = Folders[folderIndex];
    const CProxyFile2 &file = Files[folder.ArcIndex];
    pathParts.Insert(0, file.Name);
    int par = file.Parent;
    if (par < 0)
      break;
    folderIndex = Files[par].FolderIndex;
  }
}

UString CProxyArchive2::GetFullPathPrefix(unsigned folderIndex) const
{
  return Folders[folderIndex].PathPrefix;
  /*
  UString result;
  while (folderIndex > 0)
  {
    const CProxyFile2 &file = Files[Folders[folderIndex].ArcIndex];
    result = (UString)(file.Name) + (UString)WCHAR_PATH_SEPARATOR + result;
    if (file.Parent < 0)
      break;
    folderIndex = Files[file.Parent].FolderIndex;
  }
  return result;
  */
}

void CProxyArchive2::AddRealIndices_of_ArcItem(unsigned arcIndex, bool includeAltStreams, CUIntVector &realIndices) const
{
  realIndices.Add(arcIndex);
  const CProxyFile2 &file = Files[arcIndex];
  if (file.FolderIndex >= 0)
    AddRealIndices_of_Folder(file.FolderIndex, includeAltStreams, realIndices);
  if (includeAltStreams && file.AltStreamsFolderIndex >= 0)
    AddRealIndices_of_Folder(file.AltStreamsFolderIndex, includeAltStreams, realIndices);
}

void CProxyArchive2::AddRealIndices_of_Folder(unsigned folderIndex, bool includeAltStreams, CUIntVector &realIndices) const
{
  const CRecordVector<unsigned> &subFiles = Folders[folderIndex].SubFiles;
  FOR_VECTOR (i, subFiles)
  {
    AddRealIndices_of_ArcItem(subFiles[i], includeAltStreams, realIndices);
  }
}

unsigned CProxyArchive2::GetRealIndex(unsigned folderIndex, unsigned index) const
{
  return Folders[folderIndex].SubFiles[index];
}

void CProxyArchive2::GetRealIndices(unsigned folderIndex, const UInt32 *indices, UInt32 numItems, bool includeAltStreams, CUIntVector &realIndices) const
{
  const CProxyFolder2 &folder = Folders[folderIndex];
  realIndices.Clear();
  for (UInt32 i = 0; i < numItems; i++)
  {
    AddRealIndices_of_ArcItem(folder.SubFiles[indices[i]], includeAltStreams, realIndices);
  }
  HeapSort(&realIndices.Front(), realIndices.Size());
}

void CProxyArchive2::CalculateSizes(unsigned folderIndex, IInArchive *archive)
{
  CProxyFolder2 &folder = Folders[folderIndex];
  folder.Size = folder.PackSize = 0;
  folder.NumSubFolders = 0; // folder.Folders.Size();
  folder.NumSubFiles = 0; // folder.Files.Size();
  folder.CrcIsDefined = true;
  folder.Crc = 0;
  FOR_VECTOR (i, folder.SubFiles)
  {
    UInt32 index = folder.SubFiles[i];
    UInt64 size, packSize;
    bool sizeDefined = GetSize(archive, index, kpidSize, size);
    folder.Size += size;
    GetSize(archive, index, kpidPackSize, packSize);
    folder.PackSize += packSize;
    {
      NCOM::CPropVariant prop;
      if (archive->GetProperty(index, kpidCRC, &prop) == S_OK)
      {
        if (prop.vt == VT_UI4)
          folder.Crc += prop.ulVal;
        else if (prop.vt != VT_EMPTY || size != 0 || !sizeDefined)
          folder.CrcIsDefined = false;
      }
      else
        folder.CrcIsDefined = false;
    }

    const CProxyFile2 &subFile = Files[index];
    if (subFile.FolderIndex < 0)
    {
      folder.NumSubFiles++;
    }
    else
    {
      folder.NumSubFolders++;
      CProxyFolder2 &f = Folders[subFile.FolderIndex];
      f.PathPrefix = folder.PathPrefix + subFile.Name + WCHAR_PATH_SEPARATOR;
      CalculateSizes(subFile.FolderIndex, archive);
      folder.Size += f.Size;
      folder.PackSize += f.PackSize;
      folder.NumSubFiles += f.NumSubFiles;
      folder.NumSubFolders += f.NumSubFolders;
      folder.Crc += f.Crc;
      if (!f.CrcIsDefined)
        folder.CrcIsDefined = false;
    }

    if (subFile.AltStreamsFolderIndex < 0)
    {
      // folder.NumSubFiles++;
    }
    else
    {
      // folder.NumSubFolders++;
      CProxyFolder2 &f = Folders[subFile.AltStreamsFolderIndex];
      f.PathPrefix = folder.PathPrefix + subFile.Name + L":";
      CalculateSizes(subFile.AltStreamsFolderIndex, archive);
      /*
      folder.Size += f.Size;
      folder.PackSize += f.PackSize;
      folder.NumSubFiles += f.NumSubFiles;
      folder.NumSubFolders += f.NumSubFolders;
      folder.Crc += f.Crc;
      if (!f.CrcIsDefined)
        folder.CrcIsDefined = false;
      */
    }


  }
}


bool CProxyArchive2::IsThere_SubDir(unsigned folderIndex, const UString &name) const
{
  const CRecordVector<unsigned> &subFiles = Folders[folderIndex].SubFiles;
  FOR_VECTOR (i, subFiles)
  {
    const CProxyFile2 &file = Files[subFiles[i]];
    if (file.IsDir())
      if (CompareFileNames(name, file.Name) == 0)
        return true;
  }
  return false;
}

HRESULT CProxyArchive2::Load(const CArc &arc, IProgress *progress)
{
  if (!arc.GetRawProps)
    return E_FAIL;

  // DWORD tickCount = GetTickCount();

  Folders.Clear();

  IInArchive *archive = arc.Archive;

  UInt32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));
  if (progress)
    RINOK(progress->SetTotal(numItems));
  UString fileName;
  {
    CProxyFolder2 &folder = Folders.AddNew();
    folder.ArcIndex = -1;
  }

  Files.Alloc(numItems);
  
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    if (progress && (i & 0xFFFFF) == 0)
    {
      UInt64 currentItemIndex = i;
      RINOK(progress->SetCompleted(&currentItemIndex));
    }
    
    CProxyFile2 &file = Files[i];
    
    #ifdef MY_CPU_LE
    const void *p;
    UInt32 size;
    UInt32 propType;
    RINOK(arc.GetRawProps->GetRawProp(i, kpidName, &p, &size, &propType));
    
    if (p && propType == PROP_DATA_TYPE_wchar_t_PTR_Z_LE)
    {
      file.Name = (const wchar_t *)p;
      file.NameSize = 0;
      if (size >= sizeof(wchar_t))
        file.NameSize = size / sizeof(wchar_t) - 1;
    }
    else
    #endif
    {
      NCOM::CPropVariant prop;
      RINOK(arc.Archive->GetProperty(i, kpidName, &prop));
      const wchar_t *s;
      if (prop.vt == VT_BSTR)
        s = prop.bstrVal;
      else if (prop.vt == VT_EMPTY)
        s = L"[Content]";
      else
        return E_FAIL;
      file.NameSize = MyStringLen(s);
      file.Name = new wchar_t[file.NameSize + 1];
      file.NeedDeleteName = true;
      MyStringCopy((wchar_t *)file.Name, s);
    }
    UInt32 parent = (UInt32)(Int32)-1;
    UInt32 parentType = 0;
    RINOK(arc.GetRawProps->GetParent(i, &parent, &parentType));
    file.Parent = (Int32)parent;

    if (arc.Ask_Deleted)
    {
      bool isDeleted = false;
      RINOK(Archive_IsItem_Deleted(archive, i, isDeleted));
      if (isDeleted)
      {
        // continue;
        // curItem = AddDirSubItem(curItem, (UInt32)(Int32)-1, false, L"[DELETED]");
      }
    }

    bool isFolder;
    RINOK(Archive_IsItem_Folder(archive, i, isFolder));
    
    if (isFolder)
    {
      file.FolderIndex = Folders.Size();
      CProxyFolder2 &folder = Folders.AddNew();
      folder.ArcIndex = i;
    }
    if (arc.Ask_AltStream)
      RINOK(Archive_IsItem_AltStream(archive, i, file.IsAltStream));
  }

  for (i = 0; i < numItems; i++)
  {
    CProxyFile2 &file = Files[i];
    if (file.IsAltStream)
    {
      if (file.Parent >= 0)
      {
        int &folderIndex = Files[file.Parent].AltStreamsFolderIndex;
        if (folderIndex < 0)
        {
          folderIndex = Folders.Size();
          CProxyFolder2 &folder = Folders.AddNew();
          folder.ArcIndex = file.Parent; // do we need it ???
        }
        Folders[folderIndex].SubFiles.Add(i);
      }
    }
    else
    {
      int folderIndex = GetParentFolderOfFile(i);
      if (folderIndex < 0)
        return E_FAIL;
      Folders[folderIndex].SubFiles.Add(i);
    }
  }
  CalculateSizes(0, archive);

  /*
  char s[128];
  sprintf(s, "load archive %7d ms", GetTickCount() - tickCount);
  OutputDebugStringA(s);
  */
  return S_OK;
}
