// Archive/CabIn.h

#ifndef __ARCHIVE_CAB_IN_H
#define __ARCHIVE_CAB_IN_H

#include "../../IStream.h"
#include "../../Common/InBuffer.h"
#include "CabHeader.h"
#include "CabItem.h"

namespace NArchive {
namespace NCab {

class CInArchiveException
{
public:
  enum CCauseType
  {
    kUnexpectedEndOfArchive = 0,
    kIncorrectArchive,
    kUnsupported
  } Cause;
  CInArchiveException(CCauseType cause) : Cause(cause) {}
};

struct COtherArchive
{
  AString FileName;
  AString DiskName;
};

struct CArchiveInfo
{
  Byte VersionMinor; /* cabinet file format version, minor */
  Byte VersionMajor; /* cabinet file format version, major */
  UInt16 NumFolders; /* number of CFFOLDER entries in this cabinet */
  UInt16 NumFiles;   /* number of CFFILE entries in this cabinet */
  UInt16 Flags;      /* cabinet file option indicators */
  UInt16 SetID;      /* must be the same for all cabinets in a set */
  UInt16 CabinetNumber; /* number of this cabinet file in a set */

  bool ReserveBlockPresent() const { return (Flags & NHeader::NArchive::NFlags::kReservePresent) != 0; }

  bool IsTherePrev() const { return (Flags & NHeader::NArchive::NFlags::kPrevCabinet) != 0; }
  bool IsThereNext() const { return (Flags & NHeader::NArchive::NFlags::kNextCabinet) != 0; }

  UInt16 PerCabinetAreaSize; // (optional) size of per-cabinet reserved area
  Byte PerFolderAreaSize;    // (optional) size of per-folder reserved area
  Byte PerDataBlockAreaSize; // (optional) size of per-datablock reserved area

  Byte GetDataBlockReserveSize() const { return (Byte)(ReserveBlockPresent() ? PerDataBlockAreaSize : 0); }

  COtherArchive PrevArc;
  COtherArchive NextArc;

  CArchiveInfo()
  {
    Clear();
  }

  void Clear()
  {
    PerCabinetAreaSize = 0;
    PerFolderAreaSize = 0;
    PerDataBlockAreaSize = 0;
  }
};

struct CInArchiveInfo: public CArchiveInfo
{
  UInt32 Size; /* size of this cabinet file in bytes */
  UInt32 FileHeadersOffset; // offset of the first CFFILE entry
};


struct CDatabase
{
  UInt64 StartPosition;
  CInArchiveInfo ArchiveInfo;
  CObjectVector<CFolder> Folders;
  CObjectVector<CItem> Items;
  
  void Clear()
  {
    ArchiveInfo.Clear();
    Folders.Clear();
    Items.Clear();
  }
  bool IsTherePrevFolder() const
  {
    for (int i = 0; i < Items.Size(); i++)
      if (Items[i].ContinuedFromPrev())
        return true;
    return false;
  }
  int GetNumberOfNewFolders() const
  {
    int res = Folders.Size();
    if (IsTherePrevFolder())
      res--;
    return res;
  }
  UInt32 GetFileOffset(int index) const { return Items[index].Offset; }
  UInt32 GetFileSize(int index) const { return Items[index].Size; }
};

struct CDatabaseEx: public CDatabase
{
  CMyComPtr<IInStream> Stream;
};

struct CMvItem
{
  int VolumeIndex;
  int ItemIndex;
};

class CMvDatabaseEx
{
  bool AreItemsEqual(int i1, int i2);
public:
  CObjectVector<CDatabaseEx> Volumes;
  CRecordVector<CMvItem> Items;
  CRecordVector<int> StartFolderOfVol;
  CRecordVector<int> FolderStartFileIndex;
  
  int GetFolderIndex(const CMvItem *mvi) const
  {
    const CDatabaseEx &db = Volumes[mvi->VolumeIndex];
    return StartFolderOfVol[mvi->VolumeIndex] +
        db.Items[mvi->ItemIndex].GetFolderIndex(db.Folders.Size());
  }
  void Clear()
  {
    Volumes.Clear();
    Items.Clear();
    StartFolderOfVol.Clear();
    FolderStartFileIndex.Clear();
  }
  void FillSortAndShrink();
  bool Check();
};

class CInArchive
{
  CInBuffer inBuffer;

  Byte Read8();
  UInt16 Read16();
  UInt32 Read32();
  AString SafeReadName();
  void Skip(UInt32 size);
  void ReadOtherArchive(COtherArchive &oa);

public:
  HRESULT Open(const UInt64 *searchHeaderSizeLimit, CDatabaseEx &db);
};
  
}}
  
#endif
