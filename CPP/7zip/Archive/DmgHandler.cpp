// DmgHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/Buffer.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/MyXml.h"
#include "Common/UTFConvert.h"

#include "Windows/PropVariant.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/BZip2Decoder.h"
#include "../Compress/CopyCoder.h"
#include "../Compress/ZlibDecoder.h"

// #define DMG_SHOW_RAW

// #include <stdio.h>
#define PRF(x) // x

#define Get32(p) GetBe32(p)
#define Get64(p) GetBe64(p)

static int Base64ToByte(char c)
{
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  if (c == '=') return 0;
  return -1;
}

static int Base64ToBin(Byte *dest, const char *src, int srcLen)
{
  int srcPos = 0;
  int destPos = 0;
  while (srcPos < srcLen)
  {
    Byte buf[4];
    int filled = 0;
    while (srcPos < srcLen)
    {
      int n = Base64ToByte(src[srcPos++]);
      if (n >= 0)
      {
        buf[filled++] = (Byte)n;
        if (filled == 4)
          break;
      }
    }
    if (filled >= 2) { if (dest) dest[destPos] = (buf[0] << 2) | (buf[1] >> 4); destPos++; }
    if (filled >= 3) { if (dest) dest[destPos] = (buf[1] << 4) | (buf[2] >> 2); destPos++; }
    if (filled >= 4) { if (dest) dest[destPos] = (buf[2] << 6) | (buf[3]     ); destPos++; }
  }
  return destPos;
}

static UString GetSizeString(UInt64 value)
{
  wchar_t s[32];
  wchar_t c;
  if (value < (UInt64)20000) c = 0;
  else if (value < ((UInt64)20000 << 10)) { value >>= 10; c = L'K'; }
  else if (value < ((UInt64)20000 << 20)) { value >>= 20; c = L'M'; }
  else                                    { value >>= 30; c = L'G'; }
  ConvertUInt64ToString(value, s);
  int p = MyStringLen(s);
  s[p++] = c;
  s[p++] = L'\0';
  return s;
}

namespace NArchive {
namespace NDmg {

struct CBlock
{
  UInt32 Type;
  UInt64 UnpPos;
  UInt64 UnpSize;
  UInt64 PackPos;
  UInt64 PackSize;
  
  UInt64 GetNextPackOffset() const { return PackPos + PackSize; }
};

struct CFile
{
  CByteBuffer Raw;
  UInt64 StartPos;
  CRecordVector<CBlock> Blocks;
  UInt64 GetUnpackSize() const
  {
    UInt64 size = 0;
    for (int i = 0; i < Blocks.Size(); i++)
      size += Blocks[i].UnpSize;
    return size;
  };
  UInt64 GetPackSize() const
  {
    UInt64 size = 0;
    for (int i = 0; i < Blocks.Size(); i++)
      size += Blocks[i].PackSize;
    return size;
  };

  AString Name;
};

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _inStream;

  AString _xml;
  CObjectVector<CFile> _files;
  CRecordVector<int> _fileIndices;

  HRESULT Open2(IInStream *stream);
  HRESULT Extract(IInStream *stream);
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
};

const UInt32 kXmlSizeMax = ((UInt32)1 << 31) - (1 << 14);

enum
{
  METHOD_ZERO_0 = 0,
  METHOD_COPY   = 1,
  METHOD_ZERO_2 = 2,
  METHOD_ADC    = 0x80000004,
  METHOD_ZLIB   = 0x80000005,
  METHOD_BZIP2  = 0x80000006,
  METHOD_DUMMY  = 0x7FFFFFFE,
  METHOD_END    = 0xFFFFFFFF
};

struct CMethodStat
{
  UInt32 NumBlocks;
  UInt64 PackSize;
  UInt64 UnpSize;
  CMethodStat(): NumBlocks(0), PackSize(0), UnpSize(0) {}
};

struct CMethods
{
  CRecordVector<CMethodStat> Stats;
  CRecordVector<UInt32> Types;
  void Update(const CFile &file);
  UString GetString() const;
};

void CMethods::Update(const CFile &file)
{
  for (int i = 0; i < file.Blocks.Size(); i++)
  {
    const CBlock &b = file.Blocks[i];
    int index = Types.FindInSorted(b.Type);
    if (index < 0)
    {
      index = Types.AddToUniqueSorted(b.Type);
      Stats.Insert(index, CMethodStat());
    }
    CMethodStat &m = Stats[index];
    m.PackSize += b.PackSize;
    m.UnpSize += b.UnpSize;
    m.NumBlocks++;
  }
}

UString CMethods::GetString() const
{
  UString res;
  for (int i = 0; i < Types.Size(); i++)
  {
    if (i != 0)
      res += L' ';
    wchar_t buf[32];
    const wchar_t *s;
    const CMethodStat &m = Stats[i];
    bool showPack = true;
    UInt32 type = Types[i];
    switch(type)
    {
      case METHOD_ZERO_0: s = L"zero0"; showPack = (m.PackSize != 0); break;
      case METHOD_ZERO_2: s = L"zero2"; showPack = (m.PackSize != 0); break;
      case METHOD_COPY:   s = L"copy"; showPack = (m.UnpSize != m.PackSize); break;
      case METHOD_ADC:    s = L"adc"; break;
      case METHOD_ZLIB:   s = L"zlib"; break;
      case METHOD_BZIP2:  s = L"bzip2"; break;
      default: ConvertUInt64ToString(type, buf); s = buf;
    }
    res += s;
    if (m.NumBlocks != 1)
    {
      res += L'[';
      ConvertUInt64ToString(m.NumBlocks, buf);
      res += buf;
      res += L']';
    }
    res += L'-';
    res += GetSizeString(m.UnpSize);
    if (showPack)
    {
      res += L'-';
      res += GetSizeString(m.PackSize);
    }
  }
  return res;
}

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidMethod, VT_BSTR}
};

IMP_IInArchive_Props

STATPROPSTG kArcProps[] =
{
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidNumBlocks, VT_UI4}
};

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidMethod:
    {
      CMethods m;
      for (int i = 0; i < _files.Size(); i++)
        m.Update(_files[i]);
      prop = m.GetString();
      break;
    }
    case kpidNumBlocks:
    {
      UInt64 numBlocks = 0;
      for (int i = 0; i < _files.Size(); i++)
        numBlocks += _files[i].Blocks.Size();
      prop = numBlocks;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

IMP_IInArchive_ArcProps

static int FindKeyPair(const CXmlItem &item, const AString &key, const AString &nextTag)
{
  for (int i = 0; i + 1 < item.SubItems.Size(); i++)
  {
    const CXmlItem &si = item.SubItems[i];
    if (si.IsTagged("key") && si.GetSubString() == key && item.SubItems[i + 1].IsTagged(nextTag))
      return i + 1;
  }
  return -1;
}

static AString GetStringFromKeyPair(const CXmlItem &item, const AString &key, const AString &nextTag)
{
  int index = FindKeyPair(item, key, nextTag);
  if (index >= 0)
    return item.SubItems[index].GetSubString();
  return AString();
}

HRESULT CHandler::Open2(IInStream *stream)
{
  const int HEADER_SIZE = 0x1E0;

  UInt64 headerPos;
  RINOK(stream->Seek(-HEADER_SIZE, STREAM_SEEK_END, &headerPos));
  Byte buf[HEADER_SIZE];
  RINOK(ReadStream_FALSE(stream, buf, HEADER_SIZE));
  UInt64 address1 = Get64(buf + 0);
  UInt64 address2 = Get64(buf + 0xB8);
  UInt64 size64 = Get64(buf + 0xC0);
  if (address1 != address2 || size64 >= kXmlSizeMax || size64 == 0 ||
      address1 >= headerPos || address1 + size64 > headerPos)
    return S_FALSE;
  RINOK(stream->Seek(address1, STREAM_SEEK_SET, NULL));
  size_t size = (size_t)size64;

  char *ss = _xml.GetBuffer((int)size + 1);
  RINOK(ReadStream_FALSE(stream, ss, size));
  ss[size] = 0;
  _xml.ReleaseBuffer();

  CXml xml;
  if (!xml.Parse(_xml))
    return S_FALSE;
  if (xml.Root.Name != "plist")
    return S_FALSE;
  
  int dictIndex = xml.Root.FindSubTag("dict");
  if (dictIndex < 0)
    return S_FALSE;
  
  const CXmlItem &dictItem = xml.Root.SubItems[dictIndex];
  int rfDictIndex = FindKeyPair(dictItem, "resource-fork", "dict");
  if (rfDictIndex < 0)
    return S_FALSE;
  
  const CXmlItem &rfDictItem = dictItem.SubItems[rfDictIndex];
  int arrIndex = FindKeyPair(rfDictItem, "blkx", "array");
  if (arrIndex < 0)
    return S_FALSE;

  const CXmlItem &arrItem = rfDictItem.SubItems[arrIndex];

  int i;
  for (i = 0; i < arrItem.SubItems.Size(); i++)
  {
    const CXmlItem &item = arrItem.SubItems[i];
    if (!item.IsTagged("dict"))
      continue;

    CFile file;
    file.StartPos = 0;

    int destLen;
    {
      AString dataString;
      AString name = GetStringFromKeyPair(item, "Name", "string");
      if (name.IsEmpty())
        name = GetStringFromKeyPair(item, "CFName", "string");
      file.Name = name;
      dataString = GetStringFromKeyPair(item, "Data", "data");
     
      destLen = Base64ToBin(NULL, dataString, dataString.Length());
      file.Raw.SetCapacity(destLen);
      Base64ToBin(file.Raw, dataString, dataString.Length());
    }

    if (destLen > 0xCC && Get32(file.Raw) == 0x6D697368)
    {
      PRF(printf("\n\n index = %d", _files.Size()));
      const int kRecordSize = 40;
      for (int offset = 0xCC; offset + kRecordSize <= destLen; offset += kRecordSize)
      {
        const Byte *p = (const Byte *)file.Raw + offset;
        CBlock b;
        b.Type = Get32(p);
        if (b.Type == METHOD_END)
          break;
        if (b.Type == METHOD_DUMMY)
          continue;

        b.UnpPos   = Get64(p + 0x08) << 9;
        b.UnpSize  = Get64(p + 0x10) << 9;
        b.PackPos  = Get64(p + 0x18);
        b.PackSize = Get64(p + 0x20);

        file.Blocks.Add(b);

        PRF(printf("\nType=%8x  m[1]=%8x  uPos=%8x  uSize=%7x  pPos=%8x  pSize=%7x",
            b.Type, Get32(p + 4), (UInt32)b.UnpPos, (UInt32)b.UnpSize, (UInt32)b.PackPos, (UInt32)b.PackSize));
      }
    }
    int itemIndex = _files.Add(file);
    if (file.Blocks.Size() > 0)
    {
      // if (file.Name.Find("HFS") >= 0)
        _fileIndices.Add(itemIndex);
    }
  }
  
  // PackPos for each new file is 0 in some DMG files. So we use additional StartPos

  bool allStartAreZeros = true;
  for (i = 0; i < _files.Size(); i++)
  {
    const CFile &file = _files[i];
    if (!file.Blocks.IsEmpty() && file.Blocks[0].PackPos != 0)
      allStartAreZeros = false;
  }
  UInt64 startPos = 0;
  if (allStartAreZeros)
  {
    for (i = 0; i < _files.Size(); i++)
    {
      CFile &file = _files[i];
      file.StartPos = startPos;
      if (!file.Blocks.IsEmpty())
        startPos += file.Blocks.Back().GetNextPackOffset();
    }
  }

  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  {
    Close();
    if (Open2(stream) != S_OK)
      return S_FALSE;
    _inStream = stream;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _inStream.Release();
  _fileIndices.Clear();
  _files.Clear();
  _xml.Empty();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _fileIndices.Size()
    #ifdef DMG_SHOW_RAW
    + _files.Size() + 1;
    #endif
  ;
  return S_OK;
}

#define RAW_PREFIX L"raw" WSTRING_PATH_SEPARATOR

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  
  #ifdef DMG_SHOW_RAW
  if ((int)index == _fileIndices.Size())
  {
    switch(propID)
    {
      case kpidPath:
        prop = RAW_PREFIX L"a.xml";
        break;
      case kpidSize:
      case kpidPackSize:
        prop = (UInt64)_xml.Length();
        break;
    }
  }
  else if ((int)index > _fileIndices.Size())
  {
    int rawIndex = (int)index - (_fileIndices.Size() + 1);
    switch(propID)
    {
      case kpidPath:
      {
        wchar_t s[32] = RAW_PREFIX;
        ConvertUInt64ToString(rawIndex, s + MyStringLen(s));
        prop = s;
        break;
      }
      case kpidSize:
      case kpidPackSize:
        prop = (UInt64)_files[rawIndex].Raw.GetCapacity();
        break;
    }
  }
  else
  #endif
  {
    int itemIndex = _fileIndices[index];
    const CFile &item = _files[itemIndex];
    switch(propID)
    {
      case kpidMethod:
      {
        CMethods m;
        m.Update(item);
        UString resString = m.GetString();
        if (!resString.IsEmpty())
          prop = resString;
        break;
      }
      
      // case kpidExtension: prop = L"hfs"; break;

      case kpidPath:
      {
        // break;
        UString name;
        wchar_t s[32];
        ConvertUInt64ToString(index, s);
        name = s;
        int num = 10;
        int numDigits;
        for (numDigits = 1; num < _fileIndices.Size(); numDigits++)
          num *= 10;
        while (name.Length() < numDigits)
          name = L'0' + name;

        AString subName;
        int pos1 = item.Name.Find('(');
        if (pos1 >= 0)
        {
          pos1++;
          int pos2 = item.Name.Find(')', pos1);
          if (pos2 >= 0)
          {
            subName = item.Name.Mid(pos1, pos2 - pos1);
            pos1 = subName.Find(':');
            if (pos1 >= 0)
              subName = subName.Left(pos1);
          }
        }
        subName.Trim();
        if (!subName.IsEmpty())
        {
          if (subName == "Apple_HFS")
            subName = "hfs";
          else if (subName == "Apple_HFSX")
            subName = "hfsx";
          else if (subName == "Apple_Free")
            subName = "free";
          else if (subName == "DDM")
            subName = "ddm";
          UString name2;
          ConvertUTF8ToUnicode(subName, name2);
          name += L'.';
          name += name2;
        }
        else
        {
          UString name2;
          ConvertUTF8ToUnicode(item.Name, name2);
          if (!name2.IsEmpty())
            name += L" - ";
          name += name2;
        }
        prop = name;
        break;
      }
      case kpidComment:
      {
        UString name;
        ConvertUTF8ToUnicode(item.Name, name);
        prop = name;
        break;
      }

      case kpidSize:  prop = item.GetUnpackSize(); break;
      case kpidPackSize:  prop = item.GetPackSize(); break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CAdcDecoder:
  public ICompressCoder,
  public CMyUnknownImp
{
  CLzOutWindow m_OutWindowStream;
  CInBuffer m_InStream;

  void ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InStream.ReleaseStream();
  }

  class CCoderReleaser
  {
    CAdcDecoder *m_Coder;
  public:
    bool NeedFlush;
    CCoderReleaser(CAdcDecoder *coder): m_Coder(coder), NeedFlush(true) {}
    ~CCoderReleaser()
    {
      if (NeedFlush)
        m_Coder->m_OutWindowStream.Flush();
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

public:
  MY_UNKNOWN_IMP

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
};

STDMETHODIMP CAdcDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (!m_OutWindowStream.Create(1 << 18))
    return E_OUTOFMEMORY;
  if (!m_InStream.Create(1 << 18))
    return E_OUTOFMEMORY;

  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(false);
  m_InStream.SetStream(inStream);
  m_InStream.Init();
  
  CCoderReleaser coderReleaser(this);

  const UInt32 kStep = (1 << 20);
  UInt64 nextLimit = kStep;

  UInt64 pos = 0;
  while (pos < *outSize)
  {
    if (pos > nextLimit && progress)
    {
      UInt64 packSize = m_InStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &pos));
      nextLimit += kStep;
    }
    Byte b;
    if (!m_InStream.ReadByte(b))
      return S_FALSE;
    UInt64 rem = *outSize - pos;
    if (b & 0x80)
    {
      unsigned num = (b & 0x7F) + 1;
      if (num > rem)
        return S_FALSE;
      for (unsigned i = 0; i < num; i++)
      {
        if (!m_InStream.ReadByte(b))
          return S_FALSE;
        m_OutWindowStream.PutByte(b);
      }
      pos += num;
      continue;
    }
    Byte b1;
    if (!m_InStream.ReadByte(b1))
      return S_FALSE;

    UInt32 len, distance;

    if (b & 0x40)
    {
      len = ((UInt32)b & 0x3F) + 4;
      Byte b2;
      if (!m_InStream.ReadByte(b2))
        return S_FALSE;
      distance = ((UInt32)b1 << 8) + b2;
    }
    else
    {
      b &= 0x3F;
      len = ((UInt32)b >> 2) + 3;
      distance = (((UInt32)b & 3) << 8) + b1;
    }

    if (distance >= pos || len > rem)
      return S_FALSE;
    m_OutWindowStream.CopyBlock(distance, len);
    pos += len;
  }
  if (*inSize != m_InStream.GetProcessedSize())
    return S_FALSE;
  coderReleaser.NeedFlush = false;
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CAdcDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress);}
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}


STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _files.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    int index = (int)(allFilesMode ? i : indices[i]);
    #ifdef DMG_SHOW_RAW
    if (index == _fileIndices.Size())
      totalSize += _xml.Length();
    else if (index > _fileIndices.Size())
      totalSize += _files[index - (_fileIndices.Size() + 1)].Raw.GetCapacity();
    else
    #endif
      totalSize += _files[_fileIndices[index]].GetUnpackSize();
  }
  extractCallback->SetTotal(totalSize);

  UInt64 currentPackTotal = 0;
  UInt64 currentUnpTotal = 0;
  UInt64 currentPackSize = 0;
  UInt64 currentUnpSize = 0;

  const UInt32 kZeroBufSize = (1 << 14);
  CByteBuffer zeroBuf;
  zeroBuf.SetCapacity(kZeroBufSize);
  memset(zeroBuf, 0, kZeroBufSize);
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  NCompress::NBZip2::CDecoder *bzip2CoderSpec = new NCompress::NBZip2::CDecoder();
  CMyComPtr<ICompressCoder> bzip2Coder = bzip2CoderSpec;

  NCompress::NZlib::CDecoder *zlibCoderSpec = new NCompress::NZlib::CDecoder();
  CMyComPtr<ICompressCoder> zlibCoder = zlibCoderSpec;

  CAdcDecoder *adcCoderSpec = new CAdcDecoder();
  CMyComPtr<ICompressCoder> adcCoder = adcCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_inStream);

  for (i = 0; i < numItems; i++, currentPackTotal += currentPackSize, currentUnpTotal += currentUnpSize)
  {
    lps->InSize = currentPackTotal;
    lps->OutSize = currentUnpTotal;
    currentPackSize = 0;
    currentUnpSize = 0;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    // const CItemEx &item = _files[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    
    
    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));

    CLimitedSequentialOutStream *outStreamSpec = new CLimitedSequentialOutStream;
    CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
    outStreamSpec->SetStream(realOutStream);
    
    realOutStream.Release();

    Int32 opRes = NExtract::NOperationResult::kOK;
    #ifdef DMG_SHOW_RAW
    if (index > _fileIndices.Size())
    {
      const CByteBuffer &buf = _files[index - (_fileIndices.Size() + 1)].Raw;
      outStreamSpec->Init(buf.GetCapacity());
      RINOK(WriteStream(outStream, buf, buf.GetCapacity()));
      currentPackSize = currentUnpSize = buf.GetCapacity();
    }
    else if (index == _fileIndices.Size())
    {
      outStreamSpec->Init(_xml.Length());
      RINOK(WriteStream(outStream, (const char *)_xml, _xml.Length()));
      currentPackSize = currentUnpSize = _xml.Length();
    }
    else
    #endif
    {
      const CFile &item = _files[_fileIndices[index]];
      currentPackSize = item.GetPackSize();
      currentUnpSize = item.GetUnpackSize();

      UInt64 unpPos = 0;
      UInt64 packPos = 0;
      {
        for (int j = 0; j < item.Blocks.Size(); j++)
        {
          lps->InSize = currentPackTotal + packPos;
          lps->OutSize = currentUnpTotal + unpPos;
          RINOK(lps->SetCur());

          const CBlock &block = item.Blocks[j];

          packPos += block.PackSize;
          if (block.UnpPos != unpPos)
          {
            opRes = NExtract::NOperationResult::kDataError;
            break;
          }

          RINOK(_inStream->Seek(item.StartPos + block.PackPos, STREAM_SEEK_SET, NULL));
          streamSpec->Init(block.PackSize);
          // UInt64 startSize = outStreamSpec->GetSize();
          bool realMethod = true;
          outStreamSpec->Init(block.UnpSize);
          HRESULT res = S_OK;

          switch(block.Type)
          {
            case METHOD_ZERO_0:
            case METHOD_ZERO_2:
              realMethod = false;
              if (block.PackSize != 0)
                opRes = NExtract::NOperationResult::kUnSupportedMethod;
              break;

            case METHOD_COPY:
              if (block.UnpSize != block.PackSize)
              {
                opRes = NExtract::NOperationResult::kUnSupportedMethod;
                break;
              }
              res = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
              break;
            
            case METHOD_ADC:
            {
              res = adcCoder->Code(inStream, outStream, &block.PackSize, &block.UnpSize, progress);
              break;
            }
            
            case METHOD_ZLIB:
            {
              res = zlibCoder->Code(inStream, outStream, NULL, NULL, progress);
              break;
            }

            case METHOD_BZIP2:
            {
              res = bzip2Coder->Code(inStream, outStream, NULL, NULL, progress);
              if (res == S_OK)
                if (streamSpec->GetSize() != block.PackSize)
                  opRes = NExtract::NOperationResult::kDataError;
              break;
            }
            
            default:
              opRes = NExtract::NOperationResult::kUnSupportedMethod;
              break;
          }
          if (res != S_OK)
          {
            if (res != S_FALSE)
              return res;
            if (opRes == NExtract::NOperationResult::kOK)
              opRes = NExtract::NOperationResult::kDataError;
          }
          unpPos += block.UnpSize;
          if (!outStreamSpec->IsFinishedOK())
          {
            if (realMethod && opRes == NExtract::NOperationResult::kOK)
              opRes = NExtract::NOperationResult::kDataError;

            while (outStreamSpec->GetRem() != 0)
            {
              UInt64 rem = outStreamSpec->GetRem();
              UInt32 size = (UInt32)MyMin(rem, (UInt64)kZeroBufSize);
              RINOK(WriteStream(outStream, zeroBuf, size));
            }
          }
        }
      }
    }
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(opRes));
  }
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"Dmg", L"dmg", 0, 0xE4, { 0 }, 0, false, CreateArc, 0 };

REGISTER_ARC(Dmg)

}}
