// Archive/NsisIn.h

#ifndef __ARCHIVE_NSIS_IN_H
#define __ARCHIVE_NSIS_IN_H

#include "Common/MyCom.h"
#include "Common/IntToString.h"
#include "Common/Buffer.h"

#include "../../IStream.h"

#include "NsisDecode.h"

#include "../../Common/CreateCoder.h"

// #define NSIS_SCRIPT

namespace NArchive {
namespace NNsis {

const int kSignatureSize = 16;
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
  AString Prefix;
  AString Name;
  UInt32 Pos;
  bool SizeIsDefined;
  bool CompressedSizeIsDefined;
  bool EstimatedSizeIsDefined;
  UInt32 Size;
  UInt32 CompressedSize;
  UInt32 EstimatedSize;
  FILETIME DateTime;
  UInt32 DictionarySize;
  bool IsCompressed;
  bool UseFilter;
  CItem(): UseFilter(false), SizeIsDefined(false), EstimatedSizeIsDefined(false), 
    IsCompressed(true), CompressedSizeIsDefined(false), Size(0) {}

  bool IsINSTDIR() const
  {
   if (Prefix.Length() < 3)
     return false;
   return true;
  }

  AString GetReducedName() const 
  {
    AString prefix = Prefix;
    if (prefix.Length() > 0)
      if (prefix[prefix.Length() - 1] != '\\')
        prefix += '\\';
    AString s2 = prefix + Name;
    const int len = 9;
    if (s2.Left(len).CompareNoCase("$INSTDIR\\") == 0)
      s2 = s2.Mid(len);
    return s2;
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
  AString ReadString(UInt32 pos);
  AString ReadString2(UInt32 pos);
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
  bool IsSolid;
  CFirstHeader FirstHeader;
  NMethodType::EEnum Method;
  bool UseFilter;
  UInt32 DictionarySize;
  bool FilterFlag;

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

UInt32 GetUInt32FromMemLE(const Byte *p);
  
}}
  
#endif
