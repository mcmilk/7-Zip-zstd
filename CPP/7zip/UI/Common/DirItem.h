// DirItem.h

#ifndef __DIR_ITEM_H
#define __DIR_ITEM_H

#include "../../../Common/MyString.h"

#include "../../../Windows/FileFind.h"

#include "../../Common/UniqBlocks.h"

#include "../../Archive/IArchive.h"

struct CDirItem
{
  UInt64 Size;
  FILETIME CTime;
  FILETIME ATime;
  FILETIME MTime;
  UString Name;
  
  #if defined(_WIN32) && !defined(UNDER_CE)
  // UString ShortName;
  CByteBuffer ReparseData;
  CByteBuffer ReparseData2; // fixed (reduced) absolute links

  bool AreReparseData() const { return ReparseData.Size() != 0 || !ReparseData2.Size() != 0; }
  #endif
  
  UInt32 Attrib;
  int PhyParent;
  int LogParent;
  int SecureIndex;

  bool IsAltStream;
  
  CDirItem(): PhyParent(-1), LogParent(-1), SecureIndex(-1), IsAltStream(false) {}
  bool IsDir() const { return (Attrib & FILE_ATTRIBUTE_DIRECTORY) != 0 ; }
};

class CDirItems
{
  UStringVector Prefixes;
  CIntVector PhyParents;
  CIntVector LogParents;

  UString GetPrefixesPath(const CIntVector &parents, int index, const UString &name) const;

  void EnumerateDir(int phyParent, int logParent, const FString &phyPrefix);

public:
  CObjectVector<CDirItem> Items;

  bool SymLinks;

  bool ScanAltStreams;
  FStringVector ErrorPaths;
  CRecordVector<DWORD> ErrorCodes;
  UInt64 TotalSize;


  #ifndef UNDER_CE
  void SetLinkInfo(CDirItem &dirItem, const NWindows::NFile::NFind::CFileInfo &fi,
      const FString &phyPrefix);
  #endif

  void AddError(const FString &path, DWORD errorCode)
  {
    ErrorCodes.Add(errorCode);
    ErrorPaths.Add(path);
  }

  void AddError(const FString &path)
  {
    AddError(path, ::GetLastError());
  }

  #if defined(_WIN32) && !defined(UNDER_CE)

  CUniqBlocks SecureBlocks;
  CByteBuffer TempSecureBuf;
  bool _saclEnabled;
  bool ReadSecure;
  
  void AddSecurityItem(const FString &path, int &secureIndex);

  #endif

  CDirItems();

  int GetNumFolders() const { return Prefixes.Size(); }
  UString GetPhyPath(unsigned index) const;
  UString GetLogPath(unsigned index) const;

  unsigned AddPrefix(int phyParent, int logParent, const UString &prefix);
  void DeleteLastPrefix();
  void EnumerateItems2(
    const FString &phyPrefix,
    const UString &logPrefix,
    const FStringVector &filePaths,
    FStringVector *requestedPaths);

  #if defined(_WIN32) && !defined(UNDER_CE)
  void FillFixedReparse();
  #endif

  void ReserveDown();
};

struct CArcItem
{
  UInt64 Size;
  FILETIME MTime;
  UString Name;
  bool IsDir;
  bool IsAltStream;
  bool SizeDefined;
  bool MTimeDefined;
  bool Censored;
  UInt32 IndexInServer;
  int TimeType;
  
  CArcItem(): IsDir(false), IsAltStream(false), SizeDefined(false), MTimeDefined(false), Censored(false), TimeType(-1) {}
};

#endif
