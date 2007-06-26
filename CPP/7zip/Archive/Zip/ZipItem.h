// Archive/ZipItem.h

#ifndef __ARCHIVE_ZIP_ITEM_H
#define __ARCHIVE_ZIP_ITEM_H

#include "Common/Types.h"
#include "Common/MyString.h"
#include "Common/Buffer.h"

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
    // size_t res = 0;
    for (int i = 0; i < SubBlocks.Size(); i++)
      if (aesField.ParseFromSubBlock(SubBlocks[i]))
        return true;
    return false;
  }

  bool HasWzAesField() const 
  {
    CWzAesExtraField aesField;
    return GetWzAesField(aesField);
  }

  void RemoveUnknownSubBlocks()
  {
    for (int i = SubBlocks.Size() - 1; i >= 0;)
    {
      const CExtraSubBlock &subBlock = SubBlocks[i];
      if (subBlock.ID != NFileHeader::NExtraID::kWzAES)
        SubBlocks.Delete(i);
      else
        i--;
    }
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
  
  bool IsEncrypted() const;
  
  bool IsImplodeBigDictionary() const;
  bool IsImplodeLiteralsOn() const;
  
  bool IsDirectory() const;
  bool IgnoreItem() const { return false; }
  UInt32 GetWinAttributes() const;
  
  bool HasDescriptor() const;
  
private:
  void SetFlagBits(int startBitNumber, int numBits, int value);
  void SetBitMask(int bitMask, bool enable);
public:
  void ClearFlags() { Flags = 0; }
  void SetEncrypted(bool encrypted);

  WORD GetCodePage() const
  {
    return  CP_OEMCP;
  }
};

class CItem: public CLocalItem
{
public:
  CVersion MadeByVersion;
  UInt16 InternalAttributes;
  UInt32 ExternalAttributes;
  
  UInt64 LocalHeaderPosition;
  
  CExtraBlock CentralExtra;
  CByteBuffer Comment;

  bool FromLocal;
  bool FromCentral;
  
  bool IsDirectory() const;
  UInt32 GetWinAttributes() const;

  bool IsThereCrc() const 
  {
    if (CompressionMethod == NFileHeader::NCompressionMethod::kWzAES)
    {
      CWzAesExtraField aesField;
      if (CentralExtra.GetWzAesField(aesField))
        return aesField.NeedCrc();
    }
    return true;
  }
  
  WORD GetCodePage() const
  {
    return (WORD)((MadeByVersion.HostOS == NFileHeader::NHostOS::kFAT 
        || MadeByVersion.HostOS == NFileHeader::NHostOS::kNTFS
        ) ? CP_OEMCP : CP_ACP);
  }
  CItem() : FromLocal(false), FromCentral(false) {}
};

}}

#endif


