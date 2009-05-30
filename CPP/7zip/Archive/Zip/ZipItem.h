// Archive/ZipItem.h

#ifndef __ARCHIVE_ZIP_ITEM_H
#define __ARCHIVE_ZIP_ITEM_H

#include "Common/Types.h"
#include "Common/MyString.h"
#include "Common/Buffer.h"
#include "Common/UTFConvert.h"
#include "Common/StringConvert.h"

#include "ZipHeader.h"

namespace NArchive {
namespace NZip {

struct CVersion
{
  Byte Version;
  Byte HostOS;
};

bool operator==(const CVersion &v1, const CVersion &v2);
bool operator!=(const CVersion &v1, const CVersion &v2);

struct CExtraSubBlock
{
  UInt16 ID;
  CByteBuffer Data;
  bool ExtractNtfsTime(int index, FILETIME &ft) const;
  bool ExtractUnixTime(int index, UInt32 &res) const;
};

struct CWzAesExtraField
{
  UInt16 VendorVersion; // 0x0001 - AE-1, 0x0002 - AE-2,
  // UInt16 VendorId; // "AE"
  Byte Strength; // 1 - 128-bit , 2 - 192-bit , 3 - 256-bit
  UInt16 Method;

  CWzAesExtraField(): VendorVersion(2), Strength(3), Method(0) {}

  bool NeedCrc() const { return (VendorVersion == 1); }

  bool ParseFromSubBlock(const CExtraSubBlock &sb)
  {
    if (sb.ID != NFileHeader::NExtraID::kWzAES)
      return false;
    if (sb.Data.GetCapacity() < 7)
      return false;
    const Byte *p = (const Byte *)sb.Data;
    VendorVersion = (((UInt16)p[1]) << 8) | p[0];
    if (p[2] != 'A' || p[3] != 'E')
      return false;
    Strength = p[4];
    Method = (((UInt16)p[6]) << 16) | p[5];
    return true;
  }
  void SetSubBlock(CExtraSubBlock &sb) const
  {
    sb.Data.SetCapacity(7);
    sb.ID = NFileHeader::NExtraID::kWzAES;
    Byte *p = (Byte *)sb.Data;
    p[0] = (Byte)VendorVersion;
    p[1] = (Byte)(VendorVersion >> 8);
    p[2] = 'A';
    p[3] = 'E';
    p[4] = Strength;
    p[5] = (Byte)Method;
    p[6] = (Byte)(Method >> 8);
  }
};

namespace NStrongCryptoFlags
{
  const UInt16 kDES = 0x6601;
  const UInt16 kRC2old = 0x6602;
  const UInt16 k3DES168 = 0x6603;
  const UInt16 k3DES112 = 0x6609;
  const UInt16 kAES128 = 0x660E;
  const UInt16 kAES192 = 0x660F;
  const UInt16 kAES256 = 0x6610;
  const UInt16 kRC2 = 0x6702;
  const UInt16 kBlowfish = 0x6720;
  const UInt16 kTwofish = 0x6721;
  const UInt16 kRC4 = 0x6801;
}

struct CStrongCryptoField
{
  UInt16 Format;
  UInt16 AlgId;
  UInt16 BitLen;
  UInt16 Flags;

  bool ParseFromSubBlock(const CExtraSubBlock &sb)
  {
    if (sb.ID != NFileHeader::NExtraID::kStrongEncrypt)
      return false;
    const Byte *p = (const Byte *)sb.Data;
    if (sb.Data.GetCapacity() < 8)
      return false;
    Format = (((UInt16)p[1]) << 8) | p[0];
    AlgId  = (((UInt16)p[3]) << 8) | p[2];
    BitLen = (((UInt16)p[5]) << 8) | p[4];
    Flags  = (((UInt16)p[7]) << 8) | p[6];
    return (Format == 2);
  }
};

struct CExtraBlock
{
  CObjectVector<CExtraSubBlock> SubBlocks;
  void Clear() { SubBlocks.Clear(); }
  size_t GetSize() const
  {
    size_t res = 0;
    for (int i = 0; i < SubBlocks.Size(); i++)
      res += SubBlocks[i].Data.GetCapacity() + 2 + 2;
    return res;
  }
  bool GetWzAesField(CWzAesExtraField &aesField) const
  {
    for (int i = 0; i < SubBlocks.Size(); i++)
      if (aesField.ParseFromSubBlock(SubBlocks[i]))
        return true;
    return false;
  }

  bool GetStrongCryptoField(CStrongCryptoField &f) const
  {
    for (int i = 0; i < SubBlocks.Size(); i++)
      if (f.ParseFromSubBlock(SubBlocks[i]))
        return true;
    return false;
  }

  bool HasWzAesField() const
  {
    CWzAesExtraField aesField;
    return GetWzAesField(aesField);
  }

  bool GetNtfsTime(int index, FILETIME &ft) const
  {
    for (int i = 0; i < SubBlocks.Size(); i++)
    {
      const CExtraSubBlock &sb = SubBlocks[i];
      if (sb.ID == NFileHeader::NExtraID::kNTFS)
        return sb.ExtractNtfsTime(index, ft);
    }
    return false;
  }

  bool GetUnixTime(int index, UInt32 &res) const
  {
    for (int i = 0; i < SubBlocks.Size(); i++)
    {
      const CExtraSubBlock &sb = SubBlocks[i];
      if (sb.ID == NFileHeader::NExtraID::kUnixTime)
        return sb.ExtractUnixTime(index, res);
    }
    return false;
  }

  /*
  bool HasStrongCryptoField() const
  {
    CStrongCryptoField f;
    return GetStrongCryptoField(f);
  }
  */

  void RemoveUnknownSubBlocks()
  {
    for (int i = SubBlocks.Size() - 1; i >= 0; i--)
      if (SubBlocks[i].ID != NFileHeader::NExtraID::kWzAES)
        SubBlocks.Delete(i);
  }
};


class CLocalItem
{
public:
  CVersion ExtractVersion;
  UInt16 Flags;
  UInt16 CompressionMethod;
  UInt32 Time;
  UInt32 FileCRC;
  UInt64 PackSize;
  UInt64 UnPackSize;
  
  AString Name;

  CExtraBlock LocalExtra;

  bool IsUtf8() const { return (Flags & NFileHeader::NFlags::kUtf8) != 0; }
  
  bool IsEncrypted() const { return (Flags & NFileHeader::NFlags::kEncrypted) != 0; }
  bool IsStrongEncrypted() const { return IsEncrypted() && (Flags & NFileHeader::NFlags::kStrongEncrypted) != 0; };
  bool IsAesEncrypted() const { return IsEncrypted() && (IsStrongEncrypted() || CompressionMethod == NFileHeader::NCompressionMethod::kWzAES); };
  
  bool IsLzmaEOS() const { return (Flags & NFileHeader::NFlags::kLzmaEOS) != 0; }
  
  bool IsDir() const;
  bool IgnoreItem() const { return false; }
  UInt32 GetWinAttributes() const;
  
  bool HasDescriptor() const  { return (Flags & NFileHeader::NFlags::kDescriptorUsedMask) != 0; }

  UString GetUnicodeString(const AString &s) const
  {
    UString res;
    if (IsUtf8())
      if (!ConvertUTF8ToUnicode(s, res))
        res.Empty();
    if (res.IsEmpty())
      res = MultiByteToUnicodeString(s, GetCodePage());
    return res;
  }
  
private:
  void SetFlagBits(int startBitNumber, int numBits, int value);
  void SetBitMask(int bitMask, bool enable);
public:
  void ClearFlags() { Flags = 0; }
  void SetEncrypted(bool encrypted);
  void SetUtf8(bool isUtf8);

  WORD GetCodePage() const { return  CP_OEMCP; }
};

class CItem: public CLocalItem
{
public:
  CVersion MadeByVersion;
  UInt16 InternalAttributes;
  UInt32 ExternalAttributes;
  
  UInt64 LocalHeaderPosition;
  
  FILETIME NtfsMTime;
  FILETIME NtfsATime;
  FILETIME NtfsCTime;

  CExtraBlock CentralExtra;
  CByteBuffer Comment;

  bool FromLocal;
  bool FromCentral;
  bool NtfsTimeIsDefined;
  
  bool IsDir() const;
  UInt32 GetWinAttributes() const;

  bool IsThereCrc() const
  {
    if (CompressionMethod == NFileHeader::NCompressionMethod::kWzAES)
    {
      CWzAesExtraField aesField;
      if (CentralExtra.GetWzAesField(aesField))
        return aesField.NeedCrc();
    }
    return (FileCRC != 0 || !IsDir());
  }
  
  WORD GetCodePage() const
  {
    return (WORD)((MadeByVersion.HostOS == NFileHeader::NHostOS::kFAT
        || MadeByVersion.HostOS == NFileHeader::NHostOS::kNTFS
        ) ? CP_OEMCP : CP_ACP);
  }
  CItem() : FromLocal(false), FromCentral(false), NtfsTimeIsDefined(false) {}
};

}}

#endif


