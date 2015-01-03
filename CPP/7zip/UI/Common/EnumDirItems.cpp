// EnumDirItems.cpp

#include "StdAfx.h"

#include "../../../Common/Wildcard.h"

#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileIO.h"
#include "../../../Windows/FileName.h"

#if defined(_WIN32) && !defined(UNDER_CE)
#define _USE_SECURITY_CODE
#include "../../../Windows/SecurityUtils.h"
#endif

#include "EnumDirItems.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

void AddDirFileInfo(int phyParent, int logParent, int secureIndex,
    const NFind::CFileInfo &fi, CObjectVector<CDirItem> &dirItems)
{
  CDirItem di;
  di.Size = fi.Size;
  di.CTime = fi.CTime;
  di.ATime = fi.ATime;
  di.MTime = fi.MTime;
  di.Attrib = fi.Attrib;
  di.IsAltStream = fi.IsAltStream;
  di.PhyParent = phyParent;
  di.LogParent = logParent;
  di.SecureIndex = secureIndex;
  di.Name = fs2us(fi.Name);
  #if defined(_WIN32) && !defined(UNDER_CE)
  // di.ShortName = fs2us(fi.ShortName);
  #endif
  dirItems.Add(di);
}

UString CDirItems::GetPrefixesPath(const CIntVector &parents, int index, const UString &name) const
{
  UString path;
  unsigned len = name.Len();
  int i;
  for (i = index; i >= 0; i = parents[i])
    len += Prefixes[i].Len();
  unsigned totalLen = len;
  wchar_t *p = path.GetBuffer(len);
  p[len] = 0;
  len -= name.Len();
  memcpy(p + len, (const wchar_t *)name, name.Len() * sizeof(wchar_t));
  for (i = index; i >= 0; i = parents[i])
  {
    const UString &s = Prefixes[i];
    len -= s.Len();
    memcpy(p + len, (const wchar_t *)s, s.Len() * sizeof(wchar_t));
  }
  path.ReleaseBuffer(totalLen);
  return path;
}

UString CDirItems::GetPhyPath(unsigned index) const
{
  const CDirItem &di = Items[index];
  return GetPrefixesPath(PhyParents, di.PhyParent, di.Name);
}

UString CDirItems::GetLogPath(unsigned index) const
{
  const CDirItem &di = Items[index];
  return GetPrefixesPath(LogParents, di.LogParent, di.Name);
}

void CDirItems::ReserveDown()
{
  Prefixes.ReserveDown();
  PhyParents.ReserveDown();
  LogParents.ReserveDown();
  Items.ReserveDown();
}

unsigned CDirItems::AddPrefix(int phyParent, int logParent, const UString &prefix)
{
  PhyParents.Add(phyParent);
  LogParents.Add(logParent);
  return Prefixes.Add(prefix);
}

void CDirItems::DeleteLastPrefix()
{
  PhyParents.DeleteBack();
  LogParents.DeleteBack();
  Prefixes.DeleteBack();
}

bool InitLocalPrivileges();

CDirItems::CDirItems():
    SymLinks(false),
    TotalSize(0)
    #ifdef _USE_SECURITY_CODE
    , ReadSecure(false)
    #endif
{
  #ifdef _USE_SECURITY_CODE
  _saclEnabled = InitLocalPrivileges();
  #endif
}

#ifdef _USE_SECURITY_CODE

void CDirItems::AddSecurityItem(const FString &path, int &secureIndex)
{
  secureIndex = -1;

  SECURITY_INFORMATION securInfo =
      DACL_SECURITY_INFORMATION |
      GROUP_SECURITY_INFORMATION |
      OWNER_SECURITY_INFORMATION;
  if (_saclEnabled)
    securInfo |= SACL_SECURITY_INFORMATION;

  DWORD errorCode = 0;
  DWORD secureSize;
  BOOL res = ::GetFileSecurityW(fs2us(path), securInfo, (PSECURITY_DESCRIPTOR)(Byte *)TempSecureBuf, (DWORD)TempSecureBuf.Size(), &secureSize);
  if (res)
  {
    if (secureSize == 0)
      return;
    if (secureSize > TempSecureBuf.Size())
      errorCode = ERROR_INVALID_FUNCTION;
  }
  else
  {
    errorCode = GetLastError();
    if (errorCode == ERROR_INSUFFICIENT_BUFFER)
    {
      if (secureSize <= TempSecureBuf.Size())
        errorCode = ERROR_INVALID_FUNCTION;
      else
      {
        TempSecureBuf.Alloc(secureSize);
        res = ::GetFileSecurityW(fs2us(path), securInfo, (PSECURITY_DESCRIPTOR)(Byte *)TempSecureBuf, (DWORD)TempSecureBuf.Size(), &secureSize);
        if (res)
        {
          if (secureSize != TempSecureBuf.Size())
            errorCode = ERROR_INVALID_FUNCTION;;
        }
        else
          errorCode = GetLastError();
      }
    }
  }
  if (res)
  {
    secureIndex = SecureBlocks.AddUniq(TempSecureBuf, secureSize);
    return;
  }
  if (errorCode == 0)
    errorCode = ERROR_INVALID_FUNCTION;
  AddError(path, errorCode);
}

#endif

void CDirItems::EnumerateDir(int phyParent, int logParent, const FString &phyPrefix)
{
  NFind::CEnumerator enumerator(phyPrefix + FCHAR_ANY_MASK);
  for (;;)
  {
    NFind::CFileInfo fi;
    bool found;
    if (!enumerator.Next(fi, found))
    {
      AddError(phyPrefix);
      return;
    }
    if (!found)
      break;

    int secureIndex = -1;
    #ifdef _USE_SECURITY_CODE
    if (ReadSecure)
      AddSecurityItem(phyPrefix + fi.Name, secureIndex);
    #endif
    
    AddDirFileInfo(phyParent, logParent, secureIndex, fi, Items);
    
    if (fi.IsDir())
    {
      const FString name2 = fi.Name + FCHAR_PATH_SEPARATOR;
      unsigned parent = AddPrefix(phyParent, logParent, fs2us(name2));
      EnumerateDir(parent, parent, phyPrefix + name2);
    }
  }
}

void CDirItems::EnumerateItems2(
    const FString &phyPrefix,
    const UString &logPrefix,
    const FStringVector &filePaths,
    FStringVector *requestedPaths)
{
  int phyParent = phyPrefix.IsEmpty() ? -1 : AddPrefix(-1, -1, fs2us(phyPrefix));
  int logParent = logPrefix.IsEmpty() ? -1 : AddPrefix(-1, -1, logPrefix);

  FOR_VECTOR (i, filePaths)
  {
    const FString &filePath = filePaths[i];
    NFind::CFileInfo fi;
    const FString phyPath = phyPrefix + filePath;
    if (!fi.Find(phyPath))
    {
      AddError(phyPath);
      continue;
    }
    if (requestedPaths)
      requestedPaths->Add(phyPath);

    int delimiter = filePath.ReverseFind(FCHAR_PATH_SEPARATOR);
    FString phyPrefixCur;
    int phyParentCur = phyParent;
    if (delimiter >= 0)
    {
      phyPrefixCur.SetFrom(filePath, delimiter + 1);
      phyParentCur = AddPrefix(phyParent, logParent, fs2us(phyPrefixCur));
    }

    int secureIndex = -1;
    #ifdef _USE_SECURITY_CODE
    if (ReadSecure)
      AddSecurityItem(phyPath, secureIndex);
    #endif

    AddDirFileInfo(phyParentCur, logParent, secureIndex, fi, Items);
    
    if (fi.IsDir())
    {
      const FString name2 = fi.Name + FCHAR_PATH_SEPARATOR;
      unsigned parent = AddPrefix(phyParentCur, logParent, fs2us(name2));
      EnumerateDir(parent, parent, phyPrefix + phyPrefixCur + name2);
    }
  }
  ReserveDown();
}






static HRESULT EnumerateDirItems(
    const NWildcard::CCensorNode &curNode,
    int phyParent, int logParent, const FString &phyPrefix,
    const UStringVector &addArchivePrefix,
    CDirItems &dirItems,
    bool enterToSubFolders,
    IEnumDirItemCallback *callback);

static HRESULT EnumerateDirItems_Spec(
    const NWildcard::CCensorNode &curNode,
    int phyParent, int logParent, const FString &curFolderName,
    const FString &phyPrefix,
    const UStringVector &addArchivePrefix,
    CDirItems &dirItems,
    bool enterToSubFolders,
    IEnumDirItemCallback *callback)
{
  const FString name2 = curFolderName + FCHAR_PATH_SEPARATOR;
  unsigned parent = dirItems.AddPrefix(phyParent, logParent, fs2us(name2));
  unsigned numItems = dirItems.Items.Size();
  HRESULT res = EnumerateDirItems(
      curNode, parent, parent, phyPrefix + name2,
      addArchivePrefix, dirItems, enterToSubFolders, callback);
  if (numItems == dirItems.Items.Size())
    dirItems.DeleteLastPrefix();
  return res;
}

#ifndef UNDER_CE

static void EnumerateAltStreams(
    const NFind::CFileInfo &fi,
    const NWildcard::CCensorNode &curNode,
    int phyParent, int logParent, const FString &phyPrefix,
    const UStringVector &addArchivePrefix,  // prefix from curNode
    CDirItems &dirItems)
{
  const FString fullPath = phyPrefix + fi.Name;
  NFind::CStreamEnumerator enumerator(fullPath);
  for (;;)
  {
    NFind::CStreamInfo si;
    bool found;
    if (!enumerator.Next(si, found))
    {
      dirItems.AddError(fullPath + FTEXT(":*"), (DWORD)E_FAIL);
      break;
    }
    if (!found)
      break;
    if (si.IsMainStream())
      continue;
    UStringVector addArchivePrefixNew = addArchivePrefix;
    UString reducedName = si.GetReducedName();
    addArchivePrefixNew.Back() += reducedName;
    if (curNode.CheckPathToRoot(false, addArchivePrefixNew, true))
      continue;
    NFind::CFileInfo fi2 = fi;
    fi2.Name += us2fs(reducedName);
    fi2.Size = si.Size;
    fi2.Attrib &= ~FILE_ATTRIBUTE_DIRECTORY;
    fi2.IsAltStream = true;
    AddDirFileInfo(phyParent, logParent, -1, fi2, dirItems.Items);
    dirItems.TotalSize += fi2.Size;
  }
}

void CDirItems::SetLinkInfo(CDirItem &dirItem, const NFind::CFileInfo &fi,
    const FString &phyPrefix)
{
  if (!SymLinks || !fi.HasReparsePoint())
    return;
  const FString path = phyPrefix + fi.Name;
  CByteBuffer &buf = dirItem.ReparseData;
  if (NIO::GetReparseData(path, buf))
  {
    CReparseAttr attr;
    if (attr.Parse(buf, buf.Size()))
      return;
  }
  AddError(path);
  buf.Free();
}

#endif

static HRESULT EnumerateForItem(
    NFind::CFileInfo &fi,
    const NWildcard::CCensorNode &curNode,
    int phyParent, int logParent, const FString &phyPrefix,
    const UStringVector &addArchivePrefix,  // prefix from curNode
    CDirItems &dirItems,
    bool enterToSubFolders,
    IEnumDirItemCallback *callback)
{
  const UString name = fs2us(fi.Name);
  bool enterToSubFolders2 = enterToSubFolders;
  UStringVector addArchivePrefixNew = addArchivePrefix;
  addArchivePrefixNew.Add(name);
  {
    UStringVector addArchivePrefixNewTemp(addArchivePrefixNew);
    if (curNode.CheckPathToRoot(false, addArchivePrefixNewTemp, !fi.IsDir()))
      return S_OK;
  }
  int dirItemIndex = -1;
  
  if (curNode.CheckPathToRoot(true, addArchivePrefixNew, !fi.IsDir()))
  {
    int secureIndex = -1;
    #ifdef _USE_SECURITY_CODE
    if (dirItems.ReadSecure)
      dirItems.AddSecurityItem(phyPrefix + fi.Name, secureIndex);
    #endif
    
    dirItemIndex = dirItems.Items.Size();
    AddDirFileInfo(phyParent, logParent, secureIndex, fi, dirItems.Items);
    dirItems.TotalSize += fi.Size;
    if (fi.IsDir())
      enterToSubFolders2 = true;
  }

  #ifndef UNDER_CE
  if (dirItems.ScanAltStreams)
  {
    EnumerateAltStreams(fi, curNode, phyParent, logParent, phyPrefix,
        addArchivePrefixNew, dirItems);
  }

  if (dirItemIndex >= 0)
  {
    CDirItem &dirItem = dirItems.Items[dirItemIndex];
    dirItems.SetLinkInfo(dirItem, fi, phyPrefix);
    if (dirItem.ReparseData.Size() != 0)
      return S_OK;
  }
  #endif
  
  if (!fi.IsDir())
    return S_OK;
  
  const NWildcard::CCensorNode *nextNode = 0;
  if (addArchivePrefix.IsEmpty())
  {
    int index = curNode.FindSubNode(name);
    if (index >= 0)
      nextNode = &curNode.SubNodes[index];
  }
  if (!enterToSubFolders2 && nextNode == 0)
    return S_OK;
  
  addArchivePrefixNew = addArchivePrefix;
  if (nextNode == 0)
  {
    nextNode = &curNode;
    addArchivePrefixNew.Add(name);
  }
  
  return EnumerateDirItems_Spec(
      *nextNode, phyParent, logParent, fi.Name, phyPrefix,
      addArchivePrefixNew,
      dirItems,
      enterToSubFolders2, callback);
}


static bool CanUseFsDirect(const NWildcard::CCensorNode &curNode)
{
  FOR_VECTOR (i, curNode.IncludeItems)
  {
    const NWildcard::CItem &item = curNode.IncludeItems[i];
    if (item.Recursive || item.PathParts.Size() != 1)
      return false;
    const UString &name = item.PathParts.Front();
    if (name.IsEmpty())
      return false;
    
    /* Windows doesn't support file name with wildcard.
       but if another system supports file name with wildcard,
       and wildcard mode is disabled, we can ignore wildcard in name */
    /*
    if (!item.WildcardParsing)
      continue;
    */
    if (DoesNameContainWildcard(name))
      return false;
  }
  return true;
}


static HRESULT EnumerateDirItems(
    const NWildcard::CCensorNode &curNode,
    int phyParent, int logParent, const FString &phyPrefix,
    const UStringVector &addArchivePrefix,  // prefix from curNode
    CDirItems &dirItems,
    bool enterToSubFolders,
    IEnumDirItemCallback *callback)
{
  if (!enterToSubFolders)
    if (curNode.NeedCheckSubDirs())
      enterToSubFolders = true;
  if (callback)
    RINOK(callback->ScanProgress(dirItems.GetNumFolders(), dirItems.Items.Size(), dirItems.TotalSize, fs2us(phyPrefix), true));

  // try direct_names case at first
  if (addArchivePrefix.IsEmpty() && !enterToSubFolders)
  {
    if (CanUseFsDirect(curNode))
    {
      // all names are direct (no wildcards)
      // so we don't need file_system's dir enumerator
      CRecordVector<bool> needEnterVector;
      unsigned i;

      for (i = 0; i < curNode.IncludeItems.Size(); i++)
      {
        const NWildcard::CItem &item = curNode.IncludeItems[i];
        const UString &name = item.PathParts.Front();
        const FString fullPath = phyPrefix + us2fs(name);
        NFind::CFileInfo fi;
        #ifdef _WIN32
        if (phyPrefix.IsEmpty() && item.IsDriveItem())
        {
          fi.SetAsDir();
          fi.Name = fullPath;
        }
        else
        #endif
        if (!fi.Find(fullPath))
        {
          dirItems.AddError(fullPath);
          continue;
        }
        bool isDir = fi.IsDir();
        if (isDir && !item.ForDir || !isDir && !item.ForFile)
        {
          dirItems.AddError(fullPath, (DWORD)E_FAIL);
          continue;
        }
        {
          UStringVector pathParts;
          pathParts.Add(fs2us(fi.Name));
          if (curNode.CheckPathToRoot(false, pathParts, !isDir))
            continue;
        }
        
        int secureIndex = -1;
        #ifdef _USE_SECURITY_CODE
        if (dirItems.ReadSecure)
          dirItems.AddSecurityItem(fullPath, secureIndex);
        #endif

        AddDirFileInfo(phyParent, logParent, secureIndex, fi, dirItems.Items);

        #ifndef UNDER_CE
        {
          CDirItem &dirItem = dirItems.Items.Back();
          dirItems.SetLinkInfo(dirItem, fi, phyPrefix);
          if (dirItem.ReparseData.Size() != 0)
            continue;
        }
        #endif

        dirItems.TotalSize += fi.Size;

        #ifndef UNDER_CE
        if (dirItems.ScanAltStreams)
        {
          UStringVector pathParts;
          pathParts.Add(fs2us(fi.Name));
          EnumerateAltStreams(fi, curNode, phyParent, logParent, phyPrefix,
              pathParts, dirItems);
        }
        #endif

        if (!isDir)
          continue;
        
        UStringVector addArchivePrefixNew;
        const NWildcard::CCensorNode *nextNode = 0;
        int index = curNode.FindSubNode(name);
        if (index >= 0)
        {
          for (int t = needEnterVector.Size(); t <= index; t++)
            needEnterVector.Add(true);
          needEnterVector[index] = false;
          nextNode = &curNode.SubNodes[index];
        }
        else
        {
          nextNode = &curNode;
          addArchivePrefixNew.Add(name); // don't change it to fi.Name. It's for shortnames support
        }

        RINOK(EnumerateDirItems_Spec(*nextNode, phyParent, logParent, fi.Name, phyPrefix,
            addArchivePrefixNew, dirItems, true, callback));
      }
      
      for (i = 0; i < curNode.SubNodes.Size(); i++)
      {
        if (i < needEnterVector.Size())
          if (!needEnterVector[i])
            continue;
        const NWildcard::CCensorNode &nextNode = curNode.SubNodes[i];
        const FString fullPath = phyPrefix + us2fs(nextNode.Name);
        NFind::CFileInfo fi;
        #ifdef _WIN32
        if (phyPrefix.IsEmpty() && NWildcard::IsDriveColonName(nextNode.Name))
        {
          fi.SetAsDir();
          fi.Name = fullPath;
        }
        else
        #endif
        if (!fi.Find(fullPath))
        {
          if (!nextNode.AreThereIncludeItems())
            continue;
          dirItems.AddError(fullPath);
          continue;
        }
        if (!fi.IsDir())
        {
          dirItems.AddError(fullPath, (DWORD)E_FAIL);
          continue;
        }

        RINOK(EnumerateDirItems_Spec(nextNode, phyParent, logParent, fi.Name, phyPrefix,
            UStringVector(), dirItems, false, callback));
      }

      return S_OK;
    }
  }

  #ifdef _WIN32
  #ifndef UNDER_CE

  // scan drives, if wildcard is "*:\"

  if (phyPrefix.IsEmpty() && curNode.IncludeItems.Size() > 0)
  {
    unsigned i;
    for (i = 0; i < curNode.IncludeItems.Size(); i++)
    {
      const NWildcard::CItem &item = curNode.IncludeItems[i];
      if (item.PathParts.Size() < 1)
        break;
      const UString &name = item.PathParts.Front();
      if (name.Len() != 2 || name[1] != ':')
        break;
      if (item.PathParts.Size() == 1)
        if (item.ForFile || !item.ForDir)
          break;
      if (NWildcard::IsDriveColonName(name))
        continue;
      if (name[0] != '*' && name[0] != '?')
        break;
    }
    if (i == curNode.IncludeItems.Size())
    {
      FStringVector driveStrings;
      NFind::MyGetLogicalDriveStrings(driveStrings);
      for (i = 0; i < driveStrings.Size(); i++)
      {
        FString driveName = driveStrings[i];
        if (driveName.Len() < 3 || driveName.Back() != '\\')
          return E_FAIL;
        driveName.DeleteBack();
        NFind::CFileInfo fi;
        fi.SetAsDir();
        fi.Name = driveName;

        RINOK(EnumerateForItem(fi, curNode, phyParent, logParent, phyPrefix,
            addArchivePrefix, dirItems, enterToSubFolders, callback));
      }
      return S_OK;
    }
  }
  
  #endif
  #endif

  NFind::CEnumerator enumerator(phyPrefix + FCHAR_ANY_MASK);
  for (unsigned ttt = 0; ; ttt++)
  {
    NFind::CFileInfo fi;
    bool found;
    if (!enumerator.Next(fi, found))
    {
      dirItems.AddError(phyPrefix);
      break;
    }
    if (!found)
      break;

    if (callback && (ttt & 0xFF) == 0xFF)
      RINOK(callback->ScanProgress(dirItems.GetNumFolders(), dirItems.Items.Size(), dirItems.TotalSize, fs2us(phyPrefix), true));

    RINOK(EnumerateForItem(fi, curNode, phyParent, logParent, phyPrefix,
          addArchivePrefix, dirItems, enterToSubFolders, callback));
  }
  return S_OK;
}

HRESULT EnumerateItems(
    const NWildcard::CCensor &censor,
    const NWildcard::ECensorPathMode pathMode,
    const UString &addPathPrefix,
    CDirItems &dirItems,
    IEnumDirItemCallback *callback)
{
  FOR_VECTOR (i, censor.Pairs)
  {
    const NWildcard::CPair &pair = censor.Pairs[i];
    int phyParent = pair.Prefix.IsEmpty() ? -1 : dirItems.AddPrefix(-1, -1, pair.Prefix);
    int logParent = -1;
    
    if (pathMode == NWildcard::k_AbsPath)
      logParent = phyParent;
    else
    {
      if (!addPathPrefix.IsEmpty())
        logParent = dirItems.AddPrefix(-1, -1, addPathPrefix);
    }
    
    RINOK(EnumerateDirItems(pair.Head, phyParent, logParent, us2fs(pair.Prefix), UStringVector(),
        dirItems,
        false, // enterToSubFolders
        callback));
  }
  dirItems.ReserveDown();

  #if defined(_WIN32) && !defined(UNDER_CE)
  dirItems.FillFixedReparse();
  #endif

  return S_OK;
}

#if defined(_WIN32) && !defined(UNDER_CE)

void CDirItems::FillFixedReparse()
{
  /* imagex/WIM reduces absolute pathes in links (raparse data),
     if we archive non root folder. We do same thing here */

  if (!SymLinks)
    return;
  
  FOR_VECTOR(i, Items)
  {
    CDirItem &item = Items[i];
    if (item.ReparseData.Size() == 0)
      continue;
    
    CReparseAttr attr;
    if (!attr.Parse(item.ReparseData, item.ReparseData.Size()))
      continue;
    if (attr.IsRelative())
      continue;

    const UString &link = attr.GetPath();
    if (!IsDrivePath(link))
      continue;
    // maybe we need to support networks paths also ?

    FString fullPathF;
    if (!NDir::MyGetFullPathName(us2fs(GetPhyPath(i)), fullPathF))
      continue;
    UString fullPath = fs2us(fullPathF);
    const UString logPath = GetLogPath(i);
    if (logPath.Len() >= fullPath.Len())
      continue;
    if (CompareFileNames(logPath, fullPath.RightPtr(logPath.Len())) != 0)
      continue;
    
    const UString prefix = fullPath.Left(fullPath.Len() - logPath.Len());
    if (prefix.Back() != WCHAR_PATH_SEPARATOR)
      continue;

    unsigned rootPrefixSize = GetRootPrefixSize(prefix);
    if (rootPrefixSize == 0)
      continue;
    if (rootPrefixSize == prefix.Len())
      continue; // simple case: paths are from root

    if (link.Len() <= prefix.Len())
      continue;

    if (CompareFileNames(link.Left(prefix.Len()), prefix) != 0)
      continue;

    UString newLink = prefix.Left(rootPrefixSize);
    newLink += link.Ptr(prefix.Len());

    CByteBuffer data;
    if (!FillLinkData(data, newLink, attr.IsSymLink()))
      continue;
    item.ReparseData2 = data;
  }
}

#endif
