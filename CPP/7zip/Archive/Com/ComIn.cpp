// Archive/ComIn.cpp

#include "StdAfx.h"

#include "../../../../C/Alloc.h"
#include "../../../../C/CpuArch.h"

#include "Common/IntToString.h"
#include "Common/MyCom.h"

#include "../../Common/StreamUtils.h"

#include "ComIn.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)

namespace NArchive{
namespace NCom{

static const UInt32 kSignatureSize = 8;
static const Byte kSignature[kSignatureSize] = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };

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
  return ReadStream_FALSE(inStream, buf, (UInt32)1 << sectorSizeBits);
}

static HRESULT ReadIDs(IInStream *inStream, Byte *buf, int sectorSizeBits, UInt32 sid, UInt32 *dest)
{
  RINOK(ReadSector(inStream, buf, sectorSizeBits, sid));
  UInt32 sectorSize = (UInt32)1 << sectorSizeBits;
  for (UInt32 t = 0; t < sectorSize; t += 4)
    *dest++ = Get32(buf + t);
  return S_OK;
}

static void GetFileTimeFromMem(const Byte *p, FILETIME *ft)
{
  ft->dwLowDateTime = Get32(p);
  ft->dwHighDateTime = Get32(p + 4);
}

void CItem::Parse(const Byte *p, bool mode64bit)
{
  memcpy(Name, p, kNameSizeMax);
  // NameSize = Get16(p + 64);
  Type = p[66];
  LeftDid = Get32(p + 68);
  RightDid = Get32(p + 72);
  SonDid = Get32(p + 76);
  // Flags = Get32(p + 96);
  GetFileTimeFromMem(p + 100, &CTime);
  GetFileTimeFromMem(p + 108, &MTime);
  Sid = Get32(p + 116);
  Size = Get32(p + 120);
  if (mode64bit)
    Size |= ((UInt64)Get32(p + 124) << 32);
}

void CDatabase::Clear()
{
  Fat.Free();
  MiniSids.Free();
  Mat.Free();
  Items.Clear();
  Refs.Clear();
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

static UString CompoundNameToFileName(const UString &s)
{
  UString res;
  for (int i = 0; i < s.Length(); i++)
  {
    wchar_t c = s[i];
    if (c < 0x20)
    {
      res += kCharOpenBracket;
      wchar_t buf[32];
      ConvertUInt32ToString(c, buf);
      res += buf;
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

static UString ConvertName(const Byte *p, bool &isMsi)
{
  isMsi = false;
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
  {
    isMsi = true;
    return msiName;
  }
  return CompoundNameToFileName(s);
}

static UString ConvertName(const Byte *p)
{
  bool isMsi;
  return ConvertName(p, isMsi);
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

HRESULT CDatabase::Open(IInStream *inStream)
{
  MainSubfile = -1;
  static const UInt32 kHeaderSize = 512;
  Byte p[kHeaderSize];
  RINOK(ReadStream_FALSE(inStream, p, kHeaderSize));
  if (memcmp(p, kSignature, kSignatureSize) != 0)
    return S_FALSE;
  if (Get16(p + 0x1A) > 4) // majorVer
    return S_FALSE;
  if (Get16(p + 0x1C) != 0xFFFE)
    return S_FALSE;
  int sectorSizeBits = Get16(p + 0x1E);
  bool mode64bit = (sectorSizeBits >= 12);
  int miniSectorSizeBits = Get16(p + 0x20);
  SectorSizeBits = sectorSizeBits;
  MiniSectorSizeBits = miniSectorSizeBits;

  if (sectorSizeBits > 28 || miniSectorSizeBits > 28 ||
      sectorSizeBits < 7 || miniSectorSizeBits < 2 || miniSectorSizeBits > sectorSizeBits)
    return S_FALSE;
  UInt32 numSectorsForFAT = Get32(p + 0x2C);
  LongStreamMinSize = Get32(p + 0x38);
  
  UInt32 sectSize = (UInt32)1 << (int)sectorSizeBits;

  CByteBuffer sect;
  sect.SetCapacity(sectSize);

  int ssb2 = (int)(sectorSizeBits - 2);
  UInt32 numSidsInSec = (UInt32)1 << ssb2;
  UInt32 numFatItems = numSectorsForFAT << ssb2;
  if ((numFatItems >> ssb2) != numSectorsForFAT)
    return S_FALSE;
  FatSize = numFatItems;

  {
    CUInt32Buf bat;
    UInt32 numSectorsForBat = Get32(p + 0x48);
    const UInt32 kNumHeaderBatItems = 109;
    UInt32 numBatItems = kNumHeaderBatItems + (numSectorsForBat << ssb2);
    if (numBatItems < kNumHeaderBatItems || ((numBatItems - kNumHeaderBatItems) >> ssb2) != numSectorsForBat)
      return S_FALSE;
    if (!bat.Allocate(numBatItems))
      return S_FALSE;
    UInt32 i;
    for (i = 0; i < kNumHeaderBatItems; i++)
      bat[i] = Get32(p + 0x4c + i * 4);
    UInt32 sid = Get32(p + 0x44);
    for (UInt32 s = 0; s < numSectorsForBat; s++)
    {
      RINOK(ReadIDs(inStream, sect, sectorSizeBits, sid, bat + i));
      i += numSidsInSec - 1;
      sid = bat[i];
    }
    numBatItems = i;
    
    if (!Fat.Allocate(numFatItems))
      return S_FALSE;
    UInt32 j = 0;
      
    for (i = 0; i < numFatItems; j++, i += numSidsInSec)
    {
      if (j >= numBatItems)
        return S_FALSE;
      RINOK(ReadIDs(inStream, sect, sectorSizeBits, bat[j], Fat + i));
    }
  }

  UInt32 numMatItems;
  {
    UInt32 numSectorsForMat = Get32(p + 0x40);
    numMatItems = (UInt32)numSectorsForMat << ssb2;
    if ((numMatItems >> ssb2) != numSectorsForMat)
      return S_FALSE;
    if (!Mat.Allocate(numMatItems))
      return S_FALSE;
    UInt32 i;
    UInt32 sid = Get32(p + 0x3C);
    for (i = 0; i < numMatItems; i += numSidsInSec)
    {
      RINOK(ReadIDs(inStream, sect, sectorSizeBits, sid, Mat + i));
      if (sid >= numFatItems)
        return S_FALSE;
      sid = Fat[sid];
    }
    if (sid != NFatID::kEndOfChain)
      return S_FALSE;
  }

  {
    UInt32 sid = Get32(p + 0x30);
    for (;;)
    {
      if (sid >= numFatItems)
        return S_FALSE;
      RINOK(ReadSector(inStream, sect, sectorSizeBits, sid));
      for (UInt32 i = 0; i < sectSize; i += 128)
      {
        CItem item;
        item.Parse(sect + i, mode64bit);
        Items.Add(item);
      }
      sid = Fat[sid];
      if (sid == NFatID::kEndOfChain)
        break;
    }
  }

  CItem root = Items[0];

  {
    UInt32 numSectorsInMiniStream;
    {
      UInt64 numSatSects64 = (root.Size + sectSize - 1) >> sectorSizeBits;
      if (numSatSects64 > NFatID::kMaxValue)
        return S_FALSE;
      numSectorsInMiniStream = (UInt32)numSatSects64;
    }
    NumSectorsInMiniStream = numSectorsInMiniStream;
    if (!MiniSids.Allocate(numSectorsInMiniStream))
      return S_FALSE;
    {
      UInt64 matSize64 = (root.Size + ((UInt64)1 << miniSectorSizeBits) - 1) >> miniSectorSizeBits;
      if (matSize64 > NFatID::kMaxValue)
        return S_FALSE;
      MatSize = (UInt32)matSize64;
      if (numMatItems < MatSize)
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
      MiniSids[i] = sid;
      if (sid >= numFatItems)
        return S_FALSE;
      sid = Fat[sid];
    }
  }

  RINOK(AddNode(-1, root.SonDid));
  
  unsigned numCabs = 0;
  for (int i = 0; i < Refs.Size(); i++)
  {
    const CItem &item = Items[Refs[i].Did];
    if (item.IsDir() || numCabs > 1)
      continue;
    bool isMsiName;
    UString msiName = ConvertName(item.Name, isMsiName);
    if (isMsiName && msiName.Right(4).CompareNoCase(L".cab") == 0)
    {
      numCabs++;
      MainSubfile = i;
    }
  }
  if (numCabs > 1)
    MainSubfile = -1;

  return S_OK;
}

}}
