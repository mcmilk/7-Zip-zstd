// XarHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/MyXml.h"
#include "Common/StringToInt.h"
#include "Common/UTFConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "../Compress/BZip2Decoder.h"
#include "../Compress/CopyCoder.h"
#include "../Compress/ZlibDecoder.h"

#include "Common/OutStreamWithSha1.h"

#define XAR_SHOW_RAW

#define Get16(p) GetBe16(p)
#define Get32(p) GetBe32(p)
#define Get64(p) GetBe64(p)

namespace NArchive {
namespace NXar {

struct CFile
{
  AString Name;
  AString Method;
  UInt64 Size;
  UInt64 PackSize;
  UInt64 Offset;
  
  // UInt32 mode;
  UInt64 CTime;
  UInt64 MTime;
  UInt64 ATime;
  
  bool IsDir;
  bool HasData;

  bool Sha1IsDefined;
  Byte Sha1[20];
  // bool packSha1IsDefined;
  // Byte packSha1[20];

  int Parent;

  CFile(): IsDir(false), HasData(false), Sha1IsDefined(false),
    /* packSha1IsDefined(false), */
    Parent(-1), Size(0), PackSize(0), CTime(0), MTime(0), ATime(0) {}
};

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  UInt64 _dataStartPos;
  CMyComPtr<IInStream> _inStream;
  AString _xml;
  CObjectVector<CFile> _files;

  HRESULT Open2(IInStream *stream);
  HRESULT Extract(IInStream *stream);
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
};

const UInt32 kXmlSizeMax = ((UInt32)1 << 30) - (1 << 14);

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME},
  { NULL, kpidMethod, VT_BSTR}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

static bool ParseNumber(const char *s, int size, UInt32 &res)
{
  const char *end;
  res = (UInt32)ConvertStringToUInt64(s, &end);
  return (end - s == size);
}

static bool ParseUInt64(const CXmlItem &item, const char *name, UInt64 &res)
{
  AString s = item.GetSubStringForTag(name);
  const char *end;
  res = ConvertStringToUInt64(s, &end);
  return (end - (const char *)s == s.Length());
}

static UInt64 ParseTime(const CXmlItem &item, const char *name)
{
  AString s = item.GetSubStringForTag(name);
  if (s.Length() < 20)
    return 0;
  const char *p = s;
  if (p[ 4] != '-' || p[ 7] != '-' || p[10] != 'T' ||
      p[13] != ':' || p[16] != ':' || p[19] != 'Z')
    return 0;
  UInt32 year, month, day, hour, min, sec;
  if (!ParseNumber(p,      4, year )) return 0;
  if (!ParseNumber(p + 5,  2, month)) return 0;
  if (!ParseNumber(p + 8,  2, day  )) return 0;
  if (!ParseNumber(p + 11, 2, hour )) return 0;
  if (!ParseNumber(p + 14, 2, min  )) return 0;
  if (!ParseNumber(p + 17, 2, sec  )) return 0;
  
  UInt64 numSecs;
  if (!NWindows::NTime::GetSecondsSince1601(year, month, day, hour, min, sec, numSecs))
    return 0;
  return numSecs * 10000000;
}

static bool HexToByte(char c, Byte &res)
{
  if      (c >= '0' && c <= '9') res = c - '0';
  else if (c >= 'A' && c <= 'F') res = c - 'A' + 10;
  else if (c >= 'a' && c <= 'f') res = c - 'a' + 10;
  else return false;
  return true;
}

#define METHOD_NAME_ZLIB "zlib"

static bool ParseSha1(const CXmlItem &item, const char *name, Byte *digest)
{
  int index = item.FindSubTag(name);
  if (index  < 0)
    return false;
  const CXmlItem &checkItem = item.SubItems[index];
  AString style = checkItem.GetPropertyValue("style");
  if (style == "SHA1")
  {
    AString s = checkItem.GetSubString();
    if (s.Length() != 40)
      return false;
    for (int i = 0; i < s.Length(); i += 2)
    {
      Byte b0, b1;
      if (!HexToByte(s[i], b0) || !HexToByte(s[i + 1], b1))
        return false;
      digest[i / 2] = (b0 << 4) | b1;
    }
    return true;
  }
  return false;
}

static bool AddItem(const CXmlItem &item, CObjectVector<CFile> &files, int parent)
{
  if (!item.IsTag)
    return true;
  if (item.Name == "file")
  {
    CFile file;
    file.Parent = parent;
    parent = files.Size();
    file.Name = item.GetSubStringForTag("name");
    AString type = item.GetSubStringForTag("type");
    if (type == "directory")
      file.IsDir = true;
    else if (type == "file")
      file.IsDir = false;
    else
      return false;

    int dataIndex = item.FindSubTag("data");
    if (dataIndex >= 0 && !file.IsDir)
    {
      file.HasData = true;
      const CXmlItem &dataItem = item.SubItems[dataIndex];
      if (!ParseUInt64(dataItem, "size", file.Size))
        return false;
      if (!ParseUInt64(dataItem, "length", file.PackSize))
        return false;
      if (!ParseUInt64(dataItem, "offset", file.Offset))
        return false;
      file.Sha1IsDefined = ParseSha1(dataItem, "extracted-checksum", file.Sha1);
      // file.packSha1IsDefined = ParseSha1(dataItem, "archived-checksum",  file.packSha1);
      int encodingIndex = dataItem.FindSubTag("encoding");
      if (encodingIndex >= 0)
      {
        const CXmlItem &encodingItem = dataItem.SubItems[encodingIndex];
        if (encodingItem.IsTag)
        {
          AString s = encodingItem.GetPropertyValue("style");
          if (s.Length() >= 0)
          {
            AString appl = "application/";
            if (s.Left(appl.Length()) == appl)
            {
              s = s.Mid(appl.Length());
              AString xx = "x-";
              if (s.Left(xx.Length()) == xx)
              {
                s = s.Mid(xx.Length());
                if (s == "gzip")
                  s = METHOD_NAME_ZLIB;
              }
            }
            file.Method = s;
          }
        }
      }
    }

    file.CTime = ParseTime(item, "ctime");
    file.MTime = ParseTime(item, "mtime");
    file.ATime = ParseTime(item, "atime");
    files.Add(file);
  }
  for (int i = 0; i < item.SubItems.Size(); i++)
    if (!AddItem(item.SubItems[i], files, parent))
      return false;
  return true;
}

HRESULT CHandler::Open2(IInStream *stream)
{
  UInt64 archiveStartPos;
  RINOK(stream->Seek(0, STREAM_SEEK_SET, &archiveStartPos));

  const UInt32 kHeaderSize = 0x1C;
  Byte buf[kHeaderSize];
  RINOK(ReadStream_FALSE(stream, buf, kHeaderSize));

  UInt32 size = Get16(buf + 4);
  // UInt32 ver = Get16(buf + 6); // == 0
  if (Get32(buf) != 0x78617221 || size != kHeaderSize)
    return S_FALSE;

  UInt64 packSize = Get64(buf + 8);
  UInt64 unpackSize = Get64(buf + 0x10);
  // UInt32 checkSumAlogo = Get32(buf + 0x18);

  if (unpackSize >= kXmlSizeMax)
    return S_FALSE;

  _dataStartPos = archiveStartPos + kHeaderSize + packSize;

  char *ss = _xml.GetBuffer((int)unpackSize + 1);

  NCompress::NZlib::CDecoder *zlibCoderSpec = new NCompress::NZlib::CDecoder();
  CMyComPtr<ICompressCoder> zlibCoder = zlibCoderSpec;

  CLimitedSequentialInStream *inStreamLimSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStreamLim(inStreamLimSpec);
  inStreamLimSpec->SetStream(stream);
  inStreamLimSpec->Init(packSize);

  CBufPtrSeqOutStream *outStreamLimSpec = new CBufPtrSeqOutStream;
  CMyComPtr<ISequentialOutStream> outStreamLim(outStreamLimSpec);
  outStreamLimSpec->Init((Byte *)ss, (size_t)unpackSize);

  RINOK(zlibCoder->Code(inStreamLim, outStreamLim, NULL, NULL, NULL));

  if (outStreamLimSpec->GetPos() != (size_t)unpackSize)
    return S_FALSE;

  ss[(size_t)unpackSize] = 0;
  _xml.ReleaseBuffer();

  CXml xml;
  if (!xml.Parse(_xml))
    return S_FALSE;
  
  if (!xml.Root.IsTagged("xar") || xml.Root.SubItems.Size() != 1)
    return S_FALSE;
  const CXmlItem &toc = xml.Root.SubItems[0];
  if (!toc.IsTagged("toc"))
    return S_FALSE;
  if (!AddItem(toc, _files, -1))
    return S_FALSE;
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
  _files.Clear();
  _xml.Empty();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _files.Size()
    #ifdef XAR_SHOW_RAW
    + 1
    #endif
  ;
  return S_OK;
}

static void TimeToProp(UInt64 t, NWindows::NCOM::CPropVariant &prop)
{
  if (t != 0)
  {
    FILETIME ft;
    ft.dwLowDateTime = (UInt32)(t);
    ft.dwHighDateTime = (UInt32)(t >> 32);
    prop = ft;
  }
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  
  #ifdef XAR_SHOW_RAW
  if ((int)index == _files.Size())
  {
    switch(propID)
    {
      case kpidPath: prop = L"[TOC].xml"; break;
      case kpidSize:
      case kpidPackSize: prop = (UInt64)_xml.Length(); break;
    }
  }
  else
  #endif
  {
    const CFile &item = _files[index];
    switch(propID)
    {
      case kpidMethod:
      {
        UString name;
        if (!item.Method.IsEmpty() && ConvertUTF8ToUnicode(item.Method, name))
          prop = name;
        break;
      }
      case kpidPath:
      {
        AString path;
        int cur = index;
        do
        {
          const CFile &item = _files[cur];
          AString s = item.Name;
          if (s.IsEmpty())
            s = "unknown";
          if (path.IsEmpty())
            path = s;
          else
            path = s + CHAR_PATH_SEPARATOR + path;
          cur = item.Parent;
        }
        while (cur >= 0);

        UString name;
        if (ConvertUTF8ToUnicode(path, name))
          prop = name;
        break;
      }
      
      case kpidIsDir:  prop = item.IsDir; break;
      case kpidSize:      if (!item.IsDir) prop = item.Size; break;
      case kpidPackSize:  if (!item.IsDir) prop = item.PackSize; break;
      
      case kpidMTime:  TimeToProp(item.MTime, prop); break;
      case kpidCTime:  TimeToProp(item.CTime, prop); break;
      case kpidATime:  TimeToProp(item.ATime, prop); break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
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
    #ifdef XAR_SHOW_RAW
    if (index == _files.Size())
      totalSize += _xml.Length();
    else
    #endif
      totalSize += _files[index].Size;
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

  NCompress::NZlib::CDecoder *zlibCoderSpec = new NCompress::NZlib::CDecoder();
  CMyComPtr<ICompressCoder> zlibCoder = zlibCoderSpec;
  
  NCompress::NBZip2::CDecoder *bzip2CoderSpec = new NCompress::NBZip2::CDecoder();
  CMyComPtr<ICompressCoder> bzip2Coder = bzip2CoderSpec;

  NCompress::NDeflate::NDecoder::CCOMCoder *deflateCoderSpec = new NCompress::NDeflate::NDecoder::CCOMCoder();
  CMyComPtr<ICompressCoder> deflateCoder = deflateCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *inStreamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(inStreamSpec);
  inStreamSpec->SetStream(_inStream);

  
  CLimitedSequentialOutStream *outStreamLimSpec = new CLimitedSequentialOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamLimSpec);

  COutStreamWithSha1 *outStreamSha1Spec = new COutStreamWithSha1;
  {
    CMyComPtr<ISequentialOutStream> outStreamSha1(outStreamSha1Spec);
    outStreamLimSpec->SetStream(outStreamSha1);
  }

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
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    
    if (index < _files.Size())
    {
      const CFile &item = _files[index];
      if (item.IsDir)
      {
        RINOK(extractCallback->PrepareOperation(askMode));
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
        continue;
      }
    }

    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));

    outStreamSha1Spec->SetStream(realOutStream);
    realOutStream.Release();

    Int32 opRes = NExtract::NOperationResult::kOK;
    #ifdef XAR_SHOW_RAW
    if (index == _files.Size())
    {
      outStreamSha1Spec->Init(false);
      outStreamLimSpec->Init(_xml.Length());
      RINOK(WriteStream(outStream, (const char *)_xml, _xml.Length()));
      currentPackSize = currentUnpSize = _xml.Length();
    }
    else
    #endif
    {
      const CFile &item = _files[index];
      if (item.HasData)
      {
        currentPackSize = item.PackSize;
        currentUnpSize = item.Size;
        
        RINOK(_inStream->Seek(_dataStartPos + item.Offset, STREAM_SEEK_SET, NULL));
        inStreamSpec->Init(item.PackSize);
        outStreamSha1Spec->Init(item.Sha1IsDefined);
        outStreamLimSpec->Init(item.Size);
        HRESULT res = S_OK;
        
        ICompressCoder *coder = NULL;
        if (item.Method.IsEmpty() || item.Method == "octet-stream")
          if (item.PackSize == item.Size)
            coder = copyCoder;
          else
            opRes = NExtract::NOperationResult::kUnSupportedMethod;
        else if (item.Method == METHOD_NAME_ZLIB)
          coder = zlibCoder;
        else if (item.Method == "bzip2")
          coder = bzip2Coder;
        else
          opRes = NExtract::NOperationResult::kUnSupportedMethod;
        
        if (coder)
          res = coder->Code(inStream, outStream, NULL, NULL, progress);
        
        if (res != S_OK)
        {
          if (!outStreamLimSpec->IsFinishedOK())
            opRes = NExtract::NOperationResult::kDataError;
          else if (res != S_FALSE)
            return res;
          if (opRes == NExtract::NOperationResult::kOK)
            opRes = NExtract::NOperationResult::kDataError;
        }

        if (opRes == NExtract::NOperationResult::kOK)
        {
          if (outStreamLimSpec->IsFinishedOK() &&
              outStreamSha1Spec->GetSize() == item.Size)
          {
            if (!outStreamLimSpec->IsFinishedOK())
            {
              opRes = NExtract::NOperationResult::kDataError;
            }
            else if (item.Sha1IsDefined)
            {
              Byte digest[NCrypto::NSha1::kDigestSize];
              outStreamSha1Spec->Final(digest);
              if (memcmp(digest, item.Sha1, NCrypto::NSha1::kDigestSize) != 0)
                opRes = NExtract::NOperationResult::kCRCError;
            }
          }
          else
            opRes = NExtract::NOperationResult::kDataError;
        }
      }
    }
    outStreamSha1Spec->ReleaseStream();
    RINOK(extractCallback->SetOperationResult(opRes));
  }
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new NArchive::NXar::CHandler; }

static CArcInfo g_ArcInfo =
  { L"Xar", L"xar", 0, 0xE1, { 'x', 'a', 'r', '!', 0, 0x1C }, 6, false, CreateArc, 0 };

REGISTER_ARC(Xar)

}}
