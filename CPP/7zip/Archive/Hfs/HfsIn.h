// HfsIn.h

#ifndef __ARCHIVE_HFS_IN_H
#define __ARCHIVE_HFS_IN_H

#include "Common/MyString.h"
#include "Common/Buffer.h"

namespace NArchive {
namespace NHfs {

struct CExtent
{
  UInt32 Pos;
  UInt32 NumBlocks;
};

struct CFork
{
  UInt64 Size;
  // UInt32 ClumpSize;
  UInt32 NumBlocks;
  CExtent Extents[8];
  void Parse(const Byte *p);
};

struct CVolHeader
{
  Byte Header[2];
  UInt16 Version;
  // UInt32 Attr;
  // UInt32 LastMountedVersion;
  // UInt32 JournalInfoBlock;

  UInt32 CTime;
  UInt32 MTime;
  // UInt32 BackupTime;
  // UInt32 CheckedTime;
  
  // UInt32 NumFiles;
  // UInt32 NumFolders;
  int BlockSizeLog;
  UInt32 NumBlocks;
  UInt32 NumFreeBlocks;

  // UInt32 WriteCount;
  // UInt32 FinderInfo[8];
  // UInt64 VolID;

  // CFork AllocationFile;
  CFork ExtentsFile;
  CFork CatalogFile;
  // CFork AttributesFile;
  // CFork StartupFile;

  bool IsHfsX() const { return Version > 4; }
};

inline void HfsTimeToFileTime(UInt32 hfsTime, FILETIME &ft)
{
  UInt64 v = ((UInt64)3600 * 24 * (365 * 303 + 24 * 3) + hfsTime) * 10000000;
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}

enum ERecordType
{
  RECORD_TYPE_FOLDER = 1,
  RECORD_TYPE_FILE = 2,
  RECORD_TYPE_FOLDER_THREAD = 3,
  RECORD_TYPE_FILE_THREAD = 4
};

struct CItem
{
  UString Name;
  
  UInt32 ParentID;

  UInt16 Type;
  // UInt16 Flags;
  // UInt32 Valence;
  UInt32 ID;
  UInt32 CTime;
  UInt32 MTime;
  // UInt32 AttrMTime;
  UInt32 ATime;
  // UInt32 BackupDate;

  /*
  UInt32 OwnerID;
  UInt32 GroupID;
  Byte AdminFlags;
  Byte OwnerFlags;
  UInt16 FileMode;
  union
  {
    UInt32  iNodeNum;
    UInt32  LinkCount;
    UInt32  RawDevice;
  } special;
  */

  UInt64 Size;
  UInt32 NumBlocks;
  CRecordVector<CExtent> Extents;

  bool IsDir() const { return Type == RECORD_TYPE_FOLDER; }
  CItem(): Size(0), NumBlocks(0) {}
};

struct CIdIndexPair
{
  UInt32 ID;
  int Index;
};

struct CProgressVirt
{
  virtual HRESULT SetTotal(UInt64 numFiles) PURE;
  virtual HRESULT SetCompleted(UInt64 numFiles) PURE;
};

class CDatabase
{
  // CObjectVector<CIdExtents> FileExtents;
  // CObjectVector<CIdExtents> ResExtents;
  CRecordVector<CIdIndexPair> IdToIndexMap;

  HRESULT LoadExtentFile(IInStream *inStream);
  HRESULT LoadCatalog(IInStream *inStream, CProgressVirt *progress);

  HRESULT ReadFile(const CFork &fork, CByteBuffer &buf, IInStream *inStream);
public:
  CVolHeader Header;
  CObjectVector<CItem> Items;
  // bool CaseSensetive;

  void Clear()
  {
    // CaseSensetive = false;
    Items.Clear();
    // FileExtents.Clear();
    // ResExtents.Clear();
    IdToIndexMap.Clear();
  }

  UString GetItemPath(int index) const;
  HRESULT Open(IInStream *inStream, CProgressVirt *progress);
};

}}
  
#endif
