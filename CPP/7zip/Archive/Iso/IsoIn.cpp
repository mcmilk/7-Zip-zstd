// Archive/IsoIn.cpp

#include "StdAfx.h"

#include "IsoIn.h"
#include "IsoHeader.h"

#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

namespace NArchive {
namespace NIso {
 
HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 &processedSize)
{
  return ReadStream(_stream, data, size, &processedSize);
}

Byte CInArchive::ReadByte()
{
  if (m_BufferPos >= BlockSize)
    m_BufferPos = 0;
  if (m_BufferPos == 0)
  {
    UInt32 processedSize;
    if (ReadBytes(m_Buffer, BlockSize, processedSize) != S_OK)
      throw 1;
    if (processedSize != BlockSize)
      throw 1;
  }
  Byte b = m_Buffer[m_BufferPos++];
  _position++;
  return b;
}

void CInArchive::ReadBytes(Byte *data, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
    data[i] = ReadByte();
}

void CInArchive::Skeep(size_t size)
{
  while (size-- != 0)
    ReadByte();
}

void CInArchive::SkeepZeros(size_t size)
{
  while (size-- != 0)
  {
    Byte b = ReadByte();
    if (b != 0)
      throw 1;
  }
}

UInt16 CInArchive::ReadUInt16Spec()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
    value |= ((UInt16)(ReadByte()) << (8 * i));
  return value;
}


UInt16 CInArchive::ReadUInt16()
{
  Byte b[4];
  ReadBytes(b, 4);
  UInt32 value = 0;
  for (int i = 0; i < 2; i++)
  {
    if (b[i] != b[3 - i])
      throw 1;
    value |= ((UInt16)(b[i]) << (8 * i));
  }
  return (UInt16)value;
}

UInt32 CInArchive::ReadUInt32Le()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
    value |= ((UInt32)(ReadByte()) << (8 * i));
  return value;
}

UInt32 CInArchive::ReadUInt32Be()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    value <<= 8;
    value |= ReadByte();
  }
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  Byte b[8];
  ReadBytes(b, 8);
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    if (b[i] != b[7 - i])
      throw 1;
    value |= ((UInt32)(b[i]) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::ReadDigits(int numDigits)
{
  UInt32 res = 0;
  for (int i = 0; i < numDigits; i++)
  {
    Byte b = ReadByte();
    if (b < '0' || b > '9')
    {
      if (b == 0) // it's bug in some CD's
        b = '0';
      else
        throw 1;
    }
    UInt32 d = (UInt32)(b - '0');
    res *= 10;
    res += d;
  }
  return res;
}

void CInArchive::ReadDateTime(CDateTime &d)
{
  d.Year = (UInt16)ReadDigits(4);
  d.Month = (Byte)ReadDigits(2);
  d.Day = (Byte)ReadDigits(2);
  d.Hour = (Byte)ReadDigits(2);
  d.Minute = (Byte)ReadDigits(2);
  d.Second = (Byte)ReadDigits(2);
  d.Hundredths = (Byte)ReadDigits(2);
  d.GmtOffset = (signed char)ReadByte();
}

void CInArchive::ReadBootRecordDescriptor(CBootRecordDescriptor &d)
{
  ReadBytes(d.BootSystemId, sizeof(d.BootSystemId));
  ReadBytes(d.BootId, sizeof(d.BootId));
  ReadBytes(d.BootSystemUse, sizeof(d.BootSystemUse));
}

void CInArchive::ReadRecordingDateTime(CRecordingDateTime &t)
{
  t.Year = ReadByte();
  t.Month = ReadByte();
  t.Day = ReadByte();
  t.Hour = ReadByte();
  t.Minute = ReadByte();
  t.Second = ReadByte();
  t.GmtOffset = (signed char)ReadByte();
}

void CInArchive::ReadDirRecord2(CDirRecord &r, Byte len)
{
  r.ExtendedAttributeRecordLen = ReadByte();
  if (r.ExtendedAttributeRecordLen != 0)
    throw 1;
  r.ExtentLocation = ReadUInt32();
  r.DataLength = ReadUInt32();
  ReadRecordingDateTime(r.DateTime);
  r.FileFlags = ReadByte();
  r.FileUnitSize = ReadByte();
  r.InterleaveGapSize = ReadByte();
  r.VolSequenceNumber = ReadUInt16();
  Byte idLen = ReadByte();
  r.FileId.SetCapacity(idLen);
  ReadBytes((Byte *)r.FileId, idLen);
  int padSize = 1 - (idLen & 1);
  
  // SkeepZeros(1 - (idLen & 1)); 
  Skeep(1 - (idLen & 1)); // it's bug in some cd's. Must be zeros

  int curPos = 33 + idLen + padSize;
  if (curPos > len)
    throw 1;
  int rem = len - curPos;
  r.SystemUse.SetCapacity(rem);
  ReadBytes((Byte *)r.SystemUse, rem);
}

void CInArchive::ReadDirRecord(CDirRecord &r)
{
  Byte len = ReadByte();
  ReadDirRecord2(r, len);
}

void CInArchive::ReadVolumeDescriptor(CVolumeDescriptor &d)
{
  d.VolFlags = ReadByte();
  ReadBytes(d.SystemId, sizeof(d.SystemId));
  ReadBytes(d.VolumeId, sizeof(d.VolumeId));
  SkeepZeros(8);
  d.VolumeSpaceSize = ReadUInt32();
  ReadBytes(d.EscapeSequence, sizeof(d.EscapeSequence));
  d.VolumeSetSize = ReadUInt16();
  d.VolumeSequenceNumber = ReadUInt16();
  d.LogicalBlockSize = ReadUInt16();
  d.PathTableSize = ReadUInt32();
  d.LPathTableLocation = ReadUInt32Le();
  d.LOptionalPathTableLocation = ReadUInt32Le();
  d.MPathTableLocation = ReadUInt32Be();
  d.MOptionalPathTableLocation = ReadUInt32Be();
  ReadDirRecord(d.RootDirRecord);
  ReadBytes(d.VolumeSetId, sizeof(d.VolumeSetId));
  ReadBytes(d.PublisherId, sizeof(d.PublisherId));
  ReadBytes(d.DataPreparerId, sizeof(d.DataPreparerId));
  ReadBytes(d.ApplicationId, sizeof(d.ApplicationId));
  ReadBytes(d.CopyrightFileId, sizeof(d.CopyrightFileId));
  ReadBytes(d.AbstractFileId, sizeof(d.AbstractFileId));
  ReadBytes(d.BibFileId, sizeof(d.BibFileId));
  ReadDateTime(d.CreationTime);
  ReadDateTime(d.ModificationTime);
  ReadDateTime(d.ExpirationTime);
  ReadDateTime(d.EffectiveTime);
  d.FileStructureVersion = ReadByte(); // = 1
  SkeepZeros(1);
  ReadBytes(d.ApplicationUse, sizeof(d.ApplicationUse));
  SkeepZeros(653);
}

static inline bool CheckDescriptorSignature(const Byte *sig)
{
  return sig[0] == 'C' && 
         sig[1] == 'D' && 
         sig[2] == '0' && 
         sig[3] == '0' && 
         sig[4] == '1';
}

void CInArchive::SeekToBlock(UInt32 blockIndex)
{
  if (_stream->Seek((UInt64)blockIndex * VolDescs[MainVolDescIndex].LogicalBlockSize, STREAM_SEEK_SET, &_position) != S_OK)
    throw 1;
  m_BufferPos = 0;
}

void CInArchive::ReadDir(CDir &d, int level)
{
  if (!d.IsDir())
    return;
  SeekToBlock(d.ExtentLocation);
  UInt64 startPos = _position;

  bool firstItem = true;
  for (;;)
  {
    UInt64 offset = _position - startPos;
    if (offset >= d.DataLength)
      break;
    Byte len = ReadByte();
    if (len == 0)
      continue;
    CDir subItem;
    ReadDirRecord2(subItem, len);
    if (firstItem && level == 0)
      IsSusp = subItem.CheckSusp(SuspSkipSize);
      
    if (!subItem.IsSystemItem())
      d._subItems.Add(subItem);

    firstItem = false;
  }
  for (int i = 0; i < d._subItems.Size(); i++)
    ReadDir(d._subItems[i], level + 1);
}

void CInArchive::CreateRefs(CDir &d)
{
  if (!d.IsDir())
    return;
  for (int i = 0; i < d._subItems.Size(); i++)
  {
    CRef ref;
    CDir &subItem = d._subItems[i];
    subItem.Parent = &d;
    ref.Dir = &d;
    ref.Index = i;
    Refs.Add(ref);
    CreateRefs(subItem);
  }
}

void CInArchive::ReadBootInfo()
{
  if (!_bootIsDefined)
    return;
  if (memcmp(_bootDesc.BootSystemId, kElToritoSpec, sizeof(_bootDesc.BootSystemId)) != 0)
    return;
  
  const Byte *p = (const Byte *)_bootDesc.BootSystemUse;
  UInt32 blockIndex = p[0] | ((UInt32)p[1] << 8) | ((UInt32)p[2] << 16) | ((UInt32)p[3] << 24);
  SeekToBlock(blockIndex);
  Byte b = ReadByte();
  if (b != NBootEntryId::kValidationEntry)
    return;
  {
    CBootValidationEntry e;
    e.PlatformId = ReadByte();
    if (ReadUInt16Spec() != 0)
      throw 1;
    ReadBytes(e.Id, sizeof(e.Id));
    /* UInt16 checkSum = */ ReadUInt16Spec();
    if (ReadByte() != 0x55)
      throw 1;
    if (ReadByte() != 0xAA)
      throw 1;
  }
  b = ReadByte();
  if (b == NBootEntryId::kInitialEntryBootable || b == NBootEntryId::kInitialEntryNotBootable)
  {
    CBootInitialEntry e;
    e.Bootable = (b == NBootEntryId::kInitialEntryBootable);
    e.BootMediaType = ReadByte();
    e.LoadSegment = ReadUInt16Spec();
    e.SystemType = ReadByte();
    if (ReadByte() != 0)
      throw 1;
    e.SectorCount = ReadUInt16Spec();
    e.LoadRBA = ReadUInt32Le();
    if (ReadByte() != 0)
      throw 1;
    BootEntries.Add(e);
  }
  else
    return;
}

HRESULT CInArchive::Open2()
{
  Clear();
  RINOK(_stream->Seek(kStartPos, STREAM_SEEK_CUR, &_position));

  bool primVolDescDefined = false;
  m_BufferPos = 0;
  BlockSize = kBlockSize;
  VolDescs.Add(CVolumeDescriptor());
  for (;;)
  {
    Byte sig[6];
    ReadBytes(sig, 6);
    if (!CheckDescriptorSignature(sig + 1))
      return S_FALSE;
    // version = 2 for ISO 9660:1999?
    Byte ver = ReadByte();
    if (ver > 2)
      throw S_FALSE;

    if (sig[0] == NVolDescType::kTerminator)
      break;
    switch(sig[0])
    {
      case NVolDescType::kBootRecord:
      {
        _bootIsDefined = true;
        ReadBootRecordDescriptor(_bootDesc);
        break;
      }
      case NVolDescType::kPrimaryVol:
      {
        if (primVolDescDefined)
          return S_FALSE;
        primVolDescDefined = true;
        CVolumeDescriptor &volDesc = VolDescs[0];
        ReadVolumeDescriptor(volDesc);
        // some burners write "Joliet" Escape Sequence to primary volume
        memset(volDesc.EscapeSequence, 0, sizeof(volDesc.EscapeSequence));
        break;
      }
      case NVolDescType::kSupplementaryVol:
      {
        CVolumeDescriptor sd;
        ReadVolumeDescriptor(sd);
        VolDescs.Add(sd);
        break;
      }
      default:
        break;
    }
  }
  MainVolDescIndex = 0;
  if (!primVolDescDefined)
    return S_FALSE;
  for (int i = VolDescs.Size() - 1; i >= 0; i--)
  {
    if (VolDescs[i].IsJoliet())
    {
      MainVolDescIndex = i;
      break;
    }
  }
  // MainVolDescIndex = 0; // to read primary volume
  if (VolDescs[MainVolDescIndex].LogicalBlockSize != kBlockSize)
    return S_FALSE;
  (CDirRecord &)_rootDir = VolDescs[MainVolDescIndex].RootDirRecord;
  ReadDir(_rootDir, 0);
  CreateRefs(_rootDir);
  ReadBootInfo();
  return S_OK;
}

HRESULT CInArchive::Open(IInStream *inStream)
{
  _stream = inStream;
  UInt64 pos;
  RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &pos));
  RINOK(_stream->Seek(0, STREAM_SEEK_END, &_archiveSize));
  RINOK(_stream->Seek(pos, STREAM_SEEK_SET, &_position));
  HRESULT res = S_FALSE;
  try { res = Open2(); }
  catch(...) { Clear(); res = S_FALSE; }
  _stream.Release();
  return res;
}

void CInArchive::Clear()
{
  Refs.Clear();
  _rootDir.Clear();
  VolDescs.Clear();
  _bootIsDefined = false;
  BootEntries.Clear();
  SuspSkipSize = 0;
  IsSusp = false;
}

}}
