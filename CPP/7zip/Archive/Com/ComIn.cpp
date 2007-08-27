// Archive/ComIn.cpp

#include "StdAfx.h"

extern "C" 
{ 
#include "../../../../C/Alloc.h"
}

#include "Common/MyCom.h"
#include "../../Common/StreamUtils.h"
#include "Common/IntToString.h"

#include "ComIn.h"

namespace NArchive{
namespace NCom{

static const UInt32 kSignatureSize = 8;
static const Byte kSignature[kSignatureSize] = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };

static HRESULT ReadBytes(ISequentialInStream *inStream, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  RINOK(ReadStream(inStream, data, size, &realProcessedSize));
  return (realProcessedSize == size) ? S_OK : S_FALSE;
}

#ifdef LITTLE_ENDIAN_UNALIGN
#define GetUi16(p) (*(const UInt16 *)(p))
#define GetUi32(p) (*(const UInt32 *)(p))
#else
#define GetUi16(p) ((p)[0] | ((UInt16)(p)[1] << 8))
#define GetUi32(p) ((p)[0] | ((UInt32)(p)[1] << 8) | ((UInt32)(p)[2] << 16) | ((UInt32)(p)[3] << 24))
#endif


void CUInt32Buf::Free()
{
  MyFree(_buf);
  _buf = 0;
}

bool CUInt32Buf::Allocate(UInt32 numItems)
{
  Free();
  if (numItems == 0)
    return true;
  size_t newSize = (size_t)numItems * sizeof(UInt32);
  if (newSize / sizeof(UInt32) != numItems)
    return false;
  _buf = (UInt32 *)MyAlloc(newSize);
  return (_buf != 0);
}

static HRESULT ReadSector(IInStream *inStream, Byte *buf, int sectorSizeBits, UInt32 sid)
{
  RINOK(inStream->Seek((((UInt64)sid + 1) << sectorSizeBits), STREAM_SEEK_SET, NULL));
  return ReadBytes(inStream, buf, (UInt32)1 << sectorSizeBits);
}

static HRESULT ReadIDs(IInStream *inStream, Byte *buf, int sectorSizeBits, UInt32 sid, UInt32 *dest)
{
  RINOK(ReadSector(inStream, buf, sectorSizeBits, sid));
  UInt32 sectorSize = (UInt32)1 << sectorSizeBits;
  for (UInt32 t = 0; t < sectorSize; t += 4)
    *dest++ = GetUi32(buf + t);
  return S_OK;
}

static void GetFileTimeFromMem(const Byte *p, FILETIME *ft)
{
  ft->dwLowDateTime = GetUi32(p);
  ft->dwHighDateTime = GetUi32(p + 4);
}

static void ReadItem(Byte *p, CItem &item, bool mode64bit)
{
  memcpy(item.Name, p, 64);
  // item.NameSize = GetUi16(p + 64);
  item.Type = p[66];
  item.LeftDid = GetUi32(p + 68);
  item.RightDid = GetUi32(p + 72);
  item.SonDid = GetUi32(p + 76);
  // item.Flags = GetUi32(p + 96);
  GetFileTimeFromMem(p + 100, &item.CreationTime);
  GetFileTimeFromMem(p + 108, &item.LastWriteTime);
  item.Sid = GetUi32(p + 116);
  item.Size = GetUi32(p + 120);
  if (mode64bit)
    item.Size |= ((UInt64)GetUi32(p + 124) << 32);
}

static const UInt32 kNoDid = 0xFFFFFFFF;

HRESULT CDatabase::AddNode(int parent, UInt32 did)
{
  if (did == kNoDid)
    return S_OK;
  if (did >= (UInt32)Items.Size())
    return S_FALSE;
  const CItem &item = Items[did];
  if (item.IsEmpty())
    return S_FALSE;
  CRef ref;
  ref.Parent = parent;
  ref.Did = did;
  int index = Refs.Add(ref);
  if (Refs.Size() > Items.Size())
    return S_FALSE;
  RINOK(AddNode(parent, item.LeftDid));
  RINOK(AddNode(parent, item.RightDid));
  if (item.IsDir())
  {
    RINOK(AddNode(index, item.SonDid));
  }
  return S_OK;
}

static const char kCharOpenBracket  = '[';
static const char kCharCloseBracket = ']';

UString DWORDToString(UInt32 val)
{
  wchar_t buf[32];
  ConvertUInt64ToString(val, buf);
  return buf;
}

static UString CompoundNameToFileName(const UString &s)
{
  UString res;
  for (int i = 0; i < s.Length(); i++)
  {
    wchar_t c = s[i];
    if (c < 0x20)
    {
      res += kCharOpenBracket;
      res += DWORDToString(c);
      res += kCharCloseBracket;
    }
    else
      res += c;
  } 
  return res;
}

static char g_MsiChars[] =
"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._";

static const wchar_t *kMsi_ID = L""; // L"{msi}";

static const int kMsiNumBits = 6;
static const UInt32 kMsiNumChars = 1 << kMsiNumBits;
static const UInt32 kMsiCharMask = kMsiNumChars - 1;
static const UInt32 kMsiStartUnicodeChar = 0x3800;
static const UInt32 kMsiUnicodeRange = kMsiNumChars * (kMsiNumChars + 1);

bool CompoundMsiNameToFileName(const UString &name, UString &resultName)
{
  resultName.Empty();
  for (int i = 0; i < name.Length(); i++)
  {
    wchar_t c =  name[i];
    if (c < kMsiStartUnicodeChar || c > kMsiStartUnicodeChar + kMsiUnicodeRange)
      return false;
    if (i == 0)
      resultName += kMsi_ID;
    c -= kMsiStartUnicodeChar;
    
    UInt32 c0 = c & kMsiCharMask;
    UInt32 c1 = c >> kMsiNumBits;

    if (c1 <= kMsiNumChars)
    {
      resultName += (wchar_t)g_MsiChars[c0];
      if (c1 == kMsiNumChars)
        break;
      resultName += (wchar_t)g_MsiChars[c1];
    }
    else
      resultName += L'!';
  }
  return true;
}

static UString ConvertName(const Byte *p)
{
  UString s;
  for (int i = 0; i < kNameSizeMax; i += 2)
  {
    wchar_t c = (p[i] | (wchar_t)p[i + 1] << 8);
    if (c == 0)
      break;
    s += c;
  }
  UString msiName;
  if (CompoundMsiNameToFileName(s, msiName))
    return msiName;
  return CompoundNameToFileName(s);
}

UString CDatabase::GetItemPath(UInt32 index) const
{
  UString s;
  while (index != kNoDid)
  {
    const CRef &ref = Refs[index];
    const CItem &item = Items[ref.Did];
    if (!s.IsEmpty())
      s = (UString)WCHAR_PATH_SEPARATOR + s;
    s = ConvertName(item.Name) + s;
    index = ref.Parent;
  }
  return s;
}

HRESULT OpenArchive(IInStream *inStream, CDatabase &db)
{
  static const UInt32 kHeaderSize = 512;
  Byte p[kHeaderSize];
  RINOK(ReadBytes(inStream, p, kHeaderSize));
  if (memcmp(p, kSignature, kSignatureSize) != 0)
    return S_FALSE;
  UInt16 majorVer = GetUi16(p + 0x1A);
  if (majorVer > 4)
    return S_FALSE;
  if (GetUi16(p + 0x1C) != 0xFFFE)
    return S_FALSE;
  UInt16 sectorSizeBits = GetUi16(p + 0x1E);
  bool mode64bit = (sectorSizeBits >= 12);
  UInt16 miniSectorSizeBits = GetUi16(p + 0x20);
  db.SectorSizeBits = sectorSizeBits;
  db.MiniSectorSizeBits = miniSectorSizeBits;

  if (sectorSizeBits > 28 || miniSectorSizeBits > 28 || 
      sectorSizeBits < 7 || miniSectorSizeBits < 2 || miniSectorSizeBits > sectorSizeBits)
    return S_FALSE;
  UInt32 numSectorsForFAT = GetUi32(p + 0x2C);
  db.LongStreamMinSize = GetUi32(p + 0x38);
  
  UInt32 sectSize = (UInt32)1 << (int)(sectorSizeBits);

  CByteBuffer sect;
  sect.SetCapacity(sectSize);

  int ssb2 = (int)(sectorSizeBits - 2);
  UInt32 numSidsInSec = (UInt32)1 << ssb2;
  UInt32 numFatItems = numSectorsForFAT << ssb2;
  if ((numFatItems >> ssb2) != numSectorsForFAT)
    return S_FALSE;
  db.FatSize = numFatItems;

  {
    CUInt32Buf bat;
    UInt32 numSectorsForBat = GetUi32(p + 0x48);
    const UInt32 kNumHeaderBatItems = 109;
    UInt32 numBatItems = kNumHeaderBatItems + (numSectorsForBat << ssb2);
    if (numBatItems < kNumHeaderBatItems || ((numBatItems - kNumHeaderBatItems) >> ssb2) != numSectorsForBat)
      return S_FALSE;
    if (!bat.Allocate(numBatItems))
      return S_FALSE;
    UInt32 i;
    for (i = 0; i < kNumHeaderBatItems; i++)
      bat[i] = GetUi32(p + 0x4c + i * 4);
    UInt32 sid = GetUi32(p + 0x44);
    for (UInt32 s = 0; s < numSectorsForBat; s++)
    {
      RINOK(ReadIDs(inStream, sect, sectorSizeBits, sid, bat + i));
      i += numSidsInSec - 1;
      sid = bat[i];
    }
    numBatItems = i;
    
    if (!db.Fat.Allocate(numFatItems))
      return S_FALSE;
    UInt32 j = 0;
      
    for (i = 0; i < numFatItems; j++, i += numSidsInSec)
    {
      if (j >= numBatItems)
        return S_FALSE;
      RINOK(ReadIDs(inStream, sect, sectorSizeBits, bat[j], db.Fat + i));
    }
  }

  UInt32 numMatItems;
  {
    UInt32 numSectorsForMat = GetUi32(p + 0x40);
    numMatItems = (UInt32)numSectorsForMat << ssb2;
    if ((numMatItems >> ssb2) != numSectorsForMat)
      return S_FALSE;
    if (!db.Mat.Allocate(numMatItems))
      return S_FALSE;
    UInt32 i;
    UInt32 sid = GetUi32(p + 0x3C);
    for (i = 0; i < numMatItems; i += numSidsInSec)
    {
      RINOK(ReadIDs(inStream, sect, sectorSizeBits, sid, db.Mat + i));
      if (sid >= numFatItems)
        return S_FALSE;
      sid = db.Fat[sid];
    }
    if (sid != NFatID::kEndOfChain)
      return S_FALSE;
  }

  {
    UInt32 sid = GetUi32(p + 0x30);
    for (;;)
    {
      if (sid >= numFatItems)
        return S_FALSE;
      RINOK(ReadSector(inStream, sect, sectorSizeBits, sid));
      for (UInt32 i = 0; i < sectSize; i += 128)
      {
        CItem item;
        ReadItem(sect + i, item, mode64bit);
        db.Items.Add(item);
      }
      sid = db.Fat[sid];
      if (sid == NFatID::kEndOfChain)
        break;
    }
  }

  CItem root = db.Items[0];

  {
    UInt32 numSectorsInMiniStream;
    {
      UInt64 numSatSects64 = (root.Size + sectSize - 1) >> sectorSizeBits;
      if (numSatSects64 > NFatID::kMaxValue)
        return S_FALSE;
      numSectorsInMiniStream = (UInt32)numSatSects64;
    }
    db.NumSectorsInMiniStream = numSectorsInMiniStream;
    if (!db.MiniSids.Allocate(numSectorsInMiniStream))
      return S_FALSE;
    {
      UInt64 matSize64 = (root.Size + ((UInt64)1 << miniSectorSizeBits) - 1) >> miniSectorSizeBits;
      if (matSize64 > NFatID::kMaxValue)
        return S_FALSE;
      db.MatSize = (UInt32)matSize64;
      if (numMatItems < db.MatSize)
        return S_FALSE;
    }


    UInt32 sid = root.Sid;
    for (UInt32 i = 0; ; i++)
    {
      if (sid == NFatID::kEndOfChain)
      {
        if (i != numSectorsInMiniStream)
          return S_FALSE;
        break;
      }
      if (i >= numSectorsInMiniStream)
        return S_FALSE;
      db.MiniSids[i] = sid;
      if (sid >= numFatItems)
        return S_FALSE;
      sid = db.Fat[sid];
    }
  }

  return db.AddNode(-1, root.SonDid);
}

}}
