// NsisIn.h

#ifndef __ARCHIVE_NSIS_IN_H
#define __ARCHIVE_NSIS_IN_H

#include "Common/Buffer.h"
#include "Common/MyCom.h"
#include "Common/StringConvert.h"

#include "NsisDecode.h"

// #define NSIS_SCRIPT

namespace NArchive {
namespace NNsis {

const int kSignatureSize = 16;
#define NSIS_SIGNATURE { 0xEF, 0xBE, 0xAD, 0xDE, 0x4E, 0x75, 0x6C, 0x6C, 0x73, 0x6F, 0x66, 0x74, 0x49, 0x6E, 0x73, 0x74}

extern Byte kSignature[kSignatureSize];

const UInt32 kFlagsMask = 0xF;
namespace NFlags
{
  const UInt32 kUninstall = 1;
  const UInt32 kSilent = 2;
  const UInt32 kNoCrc = 4;
  const UInt32 kForceCrc = 8;
}

struct CFirstHeader
{
  UInt32 Flags;
  UInt32 HeaderLength;
 
  UInt32 ArchiveSize;

  bool ThereIsCrc() const
  {
    if ((Flags & NFlags::kForceCrc ) != 0)
      return true;
    return ((Flags & NFlags::kNoCrc) == 0);
  }

  UInt32 GetDataSize() const { return ArchiveSize - (ThereIsCrc() ? 4 : 0); }
};


struct CBlockHeader
{
  UInt32 Offset;
  UInt32 Num;
};

struct CItem
{
  AString PrefixA;
  UString PrefixU;
  AString NameA;
  UString NameU;
  FILETIME MTime;
  bool IsUnicode;
  bool UseFilter;
  bool IsCompressed;
  bool SizeIsDefined;
  bool CompressedSizeIsDefined;
  bool EstimatedSizeIsDefined;
  UInt32 Pos;
  UInt32 Size;
  UInt32 CompressedSize;
  UInt32 EstimatedSize;
  UInt32 DictionarySize;
  
  CItem(): IsUnicode(false), UseFilter(false), IsCompressed(true), SizeIsDefined(false),
      CompressedSizeIsDefined(false), EstimatedSizeIsDefined(false), Size(0), DictionarySize(1) {}

  bool IsINSTDIR() const
  {
    return (PrefixA.Length() >= 3 || PrefixU.Length() >= 3);
  }

  UString GetReducedName(bool unicode) const
  {
    UString s;
    if (unicode)
      s = PrefixU;
    else
      s = MultiByteToUnicodeString(PrefixA);
    if (s.Length() > 0)
      if (s[s.Length() - 1] != L'\\')
        s += L'\\';
    if (unicode)
      s += NameU;
    else
      s += MultiByteToUnicodeString(NameA);
    const int len = 9;
    if (s.Left(len).CompareNoCase(L"$INSTDIR\\") == 0)
      s = s.Mid(len);
    return s;
  }
};

class CInArchive
{
  UInt64 _archiveSize;
  CMyComPtr<IInStream> _stream;

  Byte ReadByte();
  UInt32 ReadUInt32();
  HRESULT Open2(
      DECL_EXTERNAL_CODECS_LOC_VARS2
      );
  void ReadBlockHeader(CBlockHeader &bh);
  AString ReadStringA(UInt32 pos) const;
  UString ReadStringU(UInt32 pos) const;
  AString ReadString2A(UInt32 pos) const;
  UString ReadString2U(UInt32 pos) const;
  AString ReadString2(UInt32 pos) const;
  AString ReadString2Qw(UInt32 pos) const;
  HRESULT ReadEntries(const CBlockHeader &bh);
  HRESULT Parse();

  CByteBuffer _data;
  UInt64 _size;

  size_t _posInData;

  UInt32 _stringsPos;


  bool _headerIsCompressed;
  UInt32 _nonSolidStartOffset;
public:
  HRESULT Open(
      DECL_EXTERNAL_CODECS_LOC_VARS
      IInStream *inStream, const UInt64 *maxCheckStartPosition);
  void Clear();

  UInt64 StreamOffset;
  CDecoder Decoder;
  CObjectVector<CItem> Items;
  CFirstHeader FirstHeader;
  NMethodType::EEnum Method;
  UInt32 DictionarySize;
  bool IsSolid;
  bool UseFilter;
  bool FilterFlag;
  bool IsUnicode;

  #ifdef NSIS_SCRIPT
  AString Script;
  #endif
  UInt32 GetOffset() const { return IsSolid ? 4 : 0; }
  UInt64 GetDataPos(int index)
  {
    const CItem &item = Items[index];
    return GetOffset() + FirstHeader.HeaderLength + item.Pos;
  }

  UInt64 GetPosOfSolidItem(int index) const
  {
    const CItem &item = Items[index];
    return 4 + FirstHeader.HeaderLength + item.Pos;
  }
  
  UInt64 GetPosOfNonSolidItem(int index) const
  {
    const CItem &item = Items[index];
    return StreamOffset + _nonSolidStartOffset + 4  + item.Pos;
  }

  void Release()
  {
    Decoder.Release();
  }

};

}}
  
#endif
