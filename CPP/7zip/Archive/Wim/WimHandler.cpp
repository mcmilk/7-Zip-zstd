// WimHandler.cpp

#include "StdAfx.h"

#include "../../../../C/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/StringToInt.h"
#include "Common/UTFConvert.h"

#include "Windows/PropVariant.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamUtils.h"

#include "WimHandler.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

using namespace NWindows;

namespace NArchive {
namespace NWim {

#define WIM_DETAILS

static STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME},
  { NULL, kpidAttrib, VT_UI4},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidShortName, VT_BSTR}
  
  #ifdef WIM_DETAILS
  , { NULL, kpidVolume, VT_UI4}
  , { NULL, kpidOffset, VT_UI8}
  , { NULL, kpidLinks, VT_UI4}
  #endif
};

static STATPROPSTG kArcProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidUnpackVer, VT_BSTR},
  { NULL, kpidIsVolume, VT_BOOL},
  { NULL, kpidVolume, VT_UI4},
  { NULL, kpidNumVolumes, VT_UI4}
};

static bool ParseNumber64(const AString &s, UInt64 &res)
{
  const char *end;
  if (s.Left(2) == "0x")
  {
    if (s.Length() == 2)
      return false;
    res = ConvertHexStringToUInt64((const char *)s + 2, &end);
  }
  else
  {
    if (s.IsEmpty())
      return false;
    res = ConvertStringToUInt64(s, &end);
  }
  return *end == 0;
}

static bool ParseNumber32(const AString &s, UInt32 &res)
{
  UInt64 res64;
  if (!ParseNumber64(s, res64) || res64 >= ((UInt64)1 << 32))
    return false;
  res = (UInt32)res64;
  return true;
}

bool ParseTime(const CXmlItem &item, FILETIME &ft, const char *tag)
{
  int index = item.FindSubTag(tag);
  if (index >= 0)
  {
    const CXmlItem &timeItem = item.SubItems[index];
    UInt32 low = 0, high = 0;
    if (ParseNumber32(timeItem.GetSubStringForTag("LOWPART"), low) &&
        ParseNumber32(timeItem.GetSubStringForTag("HIGHPART"), high))
    {
      ft.dwLowDateTime = low;
      ft.dwHighDateTime = high;
      return true;
    }
  }
  return false;
}

void CImageInfo::Parse(const CXmlItem &item)
{
  CTimeDefined = ParseTime(item, CTime, "CREATIONTIME");
  MTimeDefined = ParseTime(item, MTime, "LASTMODIFICATIONTIME");
  NameDefined = ConvertUTF8ToUnicode(item.GetSubStringForTag("NAME"), Name);
  // IndexDefined = ParseNumber32(item.GetPropertyValue("INDEX"), Index);
}

void CXml::ToUnicode(UString &s)
{
  size_t size = Data.GetCapacity();
  if (size < 2 || (size & 1) != 0 || size > (1 << 24))
    return;
  const Byte *p = Data;
  if (Get16(p) != 0xFEFF)
    return;
  wchar_t *chars = s.GetBuffer((int)size / 2);
  for (size_t i = 2; i < size; i += 2)
    *chars++ = (wchar_t)Get16(p + i);
  *chars = 0;
  s.ReleaseBuffer();
}

void CXml::Parse()
{
  UString s;
  ToUnicode(s);
  AString utf;
  if (!ConvertUnicodeToUTF8(s, utf))
    return;
  ::CXml xml;
  if (!xml.Parse(utf))
    return;
  if (xml.Root.Name != "WIM")
    return;

  for (int i = 0; i < xml.Root.SubItems.Size(); i++)
  {
    const CXmlItem &item = xml.Root.SubItems[i];
    if (item.IsTagged("IMAGE"))
    {
      CImageInfo imageInfo;
      imageInfo.Parse(item);
      Images.Add(imageInfo);
    }
  }
}

static const char *kMethodLZX = "LZX";
static const char *kMethodXpress = "XPress";
static const char *kMethodCopy = "Copy";

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;

  const CImageInfo *image = NULL;
  if (_xmls.Size() == 1)
  {
    const CXml &xml = _xmls[0];
    if (xml.Images.Size() == 1)
      image = &xml.Images[0];
  }

  switch(propID)
  {
    case kpidSize: prop = _db.GetUnpackSize(); break;
    case kpidPackSize: prop = _db.GetPackSize(); break;
    
    case kpidCTime:
      if (_xmls.Size() == 1)
      {
        const CXml &xml = _xmls[0];
        int index = -1;
        for (int i = 0; i < xml.Images.Size(); i++)
        {
          const CImageInfo &image = xml.Images[i];
          if (image.CTimeDefined)
            if (index < 0 || ::CompareFileTime(&image.CTime, &xml.Images[index].CTime) < 0)
              index = i;
        }
        if (index >= 0)
          prop = xml.Images[index].CTime;
      }
      break;

    case kpidMTime:
      if (_xmls.Size() == 1)
      {
        const CXml &xml = _xmls[0];
        int index = -1;
        for (int i = 0; i < xml.Images.Size(); i++)
        {
          const CImageInfo &image = xml.Images[i];
          if (image.MTimeDefined)
            if (index < 0 || ::CompareFileTime(&image.MTime, &xml.Images[index].MTime) > 0)
              index = i;
        }
        if (index >= 0)
          prop = xml.Images[index].MTime;
      }
      break;

    case kpidComment:
      if (image != NULL)
      {
        if (_xmlInComments)
        {
          UString s;
          _xmls[0].ToUnicode(s);
          prop = s;
        }
        else if (image->NameDefined)
          prop = image->Name;
      }
      break;

    case kpidUnpackVer:
    {
      UInt32 ver1 = _version >> 16;
      UInt32 ver2 = (_version >> 8) & 0xFF;
      UInt32 ver3 = (_version) & 0xFF;

      char s[16];
      ConvertUInt32ToString(ver1, s);
      AString res = s;
      res += '.';
      ConvertUInt32ToString(ver2, s);
      res += s;
      if (ver3 != 0)
      {
        res += '.';
        ConvertUInt32ToString(ver3, s);
        res += s;
      }
      prop = res;
      break;
    }

    case kpidIsVolume:
      if (_xmls.Size() > 0)
      {
        UInt16 volIndex = _xmls[0].VolIndex;
        if (volIndex < _volumes.Size())
          prop = (_volumes[volIndex].Header.NumParts > 1);
      }
      break;
    case kpidVolume:
      if (_xmls.Size() > 0)
      {
        UInt16 volIndex = _xmls[0].VolIndex;
        if (volIndex < _volumes.Size())
          prop = (UInt32)_volumes[volIndex].Header.PartNumber;
      }
      break;
    case kpidNumVolumes: if (_volumes.Size() > 0) prop = (UInt32)(_volumes.Size() - 1); break;
    case kpidMethod:
    {
      bool lzx = false, xpress = false, copy = false;
      for (int i = 0; i < _xmls.Size(); i++)
      {
        const CHeader &header = _volumes[_xmls[i].VolIndex].Header;
        if (header.IsCompressed())
          if (header.IsLzxMode())
            lzx = true;
          else
            xpress = true;
        else
          copy = true;
      }
      AString res;
      if (lzx)
        res = kMethodLZX;
      if (xpress)
      {
        if (!res.IsEmpty())
          res += ' ';
        res += kMethodXpress;
      }
      if (copy)
      {
        if (!res.IsEmpty())
          res += ' ';
        res += kMethodCopy;
      }
      prop = res;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  if (index < (UInt32)_db.SortedItems.Size())
  {
    int realIndex = _db.SortedItems[index];
    const CItem &item = _db.Items[realIndex];
    const CStreamInfo *si = NULL;
    const CVolume *vol = NULL;
    if (item.StreamIndex >= 0)
    {
      si = &_db.Streams[item.StreamIndex];
      vol = &_volumes[si->PartNumber];
    }

    switch(propID)
    {
      case kpidPath:
        if (item.HasMetadata)
          prop = _db.GetItemPath(realIndex);
        else
        {
          char sz[16];
          ConvertUInt32ToString(item.StreamIndex, sz);
          AString s = sz;
          while (s.Length() < _nameLenForStreams)
            s = '0' + s;
          /*
          if (si->Resource.IsFree())
            prefix = "[Free]";
          */
          s = "[Files]" STRING_PATH_SEPARATOR + s;
          prop = s;
        }
        break;
      case kpidShortName: if (item.HasMetadata) prop = item.ShortName; break;

      case kpidIsDir: prop = item.IsDir(); break;
      case kpidAttrib: if (item.HasMetadata) prop = item.Attrib; break;
      case kpidCTime: if (item.HasMetadata) prop = item.CTime; break;
      case kpidATime: if (item.HasMetadata) prop = item.ATime; break;
      case kpidMTime: if (item.HasMetadata) prop = item.MTime; break;
      case kpidPackSize: prop = si ? si->Resource.PackSize : (UInt64)0; break;
      case kpidSize: prop = si ? si->Resource.UnpackSize : (UInt64)0; break;
      case kpidMethod: if (si) prop = si->Resource.IsCompressed() ?
          (vol->Header.IsLzxMode() ? kMethodLZX : kMethodXpress) : kMethodCopy; break;
      #ifdef WIM_DETAILS
      case kpidVolume: if (si) prop = (UInt32)si->PartNumber; break;
      case kpidOffset: if (si)  prop = (UInt64)si->Resource.Offset; break;
      case kpidLinks: prop = si ? (UInt32)si->RefCount : (UInt32)0; break;
      #endif
    }
  }
  else
  {
    index -= _db.SortedItems.Size();
    {
      switch(propID)
      {
        case kpidPath:
        {
          char sz[16];
          ConvertUInt32ToString(_xmls[index].VolIndex, sz);
          prop = (AString)"[" + (AString)sz + "].xml";
          break;
        }
        case kpidIsDir: prop = false; break;
        case kpidPackSize:
        case kpidSize: prop = (UInt64)_xmls[index].Data.GetCapacity(); break;
        case kpidMethod: prop = kMethodCopy; break;
      }
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CVolumeName
{
  // UInt32 _volIndex;
  UString _before;
  UString _after;
public:
  CVolumeName() {};

  void InitName(const UString &name)
  {
    // _volIndex = 1;
    int dotPos = name.ReverseFind('.');
    if (dotPos < 0)
      dotPos = name.Length();
    _before = name.Left(dotPos);
    _after = name.Mid(dotPos);
  }

  UString GetNextName(UInt32 index)
  {
    wchar_t s[16];
    ConvertUInt32ToString(index, s);
    return _before + (UString)s + _after;
  }
};

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  {
    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    
    CVolumeName seqName;
    if (openArchiveCallback != NULL)
      openArchiveCallback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);

    UInt32 numVolumes = 1;
    int firstVolumeIndex = -1;
    for (UInt32 i = 1; i <= numVolumes; i++)
    {
      CMyComPtr<IInStream> curStream;
      if (i != 1)
      {
        UString fullName = seqName.GetNextName(i);
        HRESULT result = openVolumeCallback->GetStream(fullName, &curStream);
        if (result == S_FALSE)
          continue;
        if (result != S_OK)
          return result;
        if (!curStream)
          break;
      }
      else
        curStream = inStream;
      CHeader header;
      HRESULT res = NWim::ReadHeader(curStream, header);
      if (res != S_OK)
      {
        if (i == 1)
          return res;
        if (res == S_FALSE)
          continue;
        return res;
      }
      _version = header.Version;
      _isOldVersion = header.IsOldVersion();
      if (firstVolumeIndex >= 0)
        if (!header.AreFromOnArchive(_volumes[firstVolumeIndex].Header))
          break;
      if (_volumes.Size() > header.PartNumber && _volumes[header.PartNumber].Stream)
        break;
      CXml xml;
      xml.VolIndex = header.PartNumber;
      res = _db.Open(curStream, header, xml.Data, openArchiveCallback);
      if (res != S_OK)
      {
        if (i == 1)
          return res;
        if (res == S_FALSE)
          continue;
        return res;
      }
      
      while (_volumes.Size() <= header.PartNumber)
        _volumes.Add(CVolume());
      CVolume &volume = _volumes[header.PartNumber];
      volume.Header = header;
      volume.Stream = curStream;
      
      firstVolumeIndex = header.PartNumber;
      
      bool needAddXml = true;
      if (_xmls.Size() != 0)
        if (xml.Data == _xmls[0].Data)
          needAddXml = false;
      if (needAddXml)
      {
        xml.Parse();
        _xmls.Add(xml);
      }
      
      if (i == 1)
      {
        if (header.PartNumber != 1)
          break;
        if (!openVolumeCallback)
          break;
        numVolumes = header.NumParts;
        {
          NCOM::CPropVariant prop;
          RINOK(openVolumeCallback->GetProperty(kpidName, &prop));
          if (prop.vt != VT_BSTR)
            break;
          seqName.InitName(prop.bstrVal);
        }
      }
    }

    _db.DetectPathMode();
    RINOK(_db.Sort(_db.SkipRoot));

    wchar_t sz[16];
    ConvertUInt32ToString(_db.Streams.Size(), sz);
    _nameLenForStreams = MyStringLen(sz);

    _xmlInComments = (_xmls.Size() == 1 && !_db.ShowImageNumber);
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _db.Clear();
  _volumes.Clear();
  _xmls.Clear();
  _nameLenForStreams = 0;
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);

  if (allFilesMode)
    numItems = _db.SortedItems.Size() + _xmls.Size();
  if (numItems == 0)
    return S_OK;

  UInt32 i;
  UInt64 totalSize = 0;
  for (i = 0; i < numItems; i++)
  {
    UInt32 index = allFilesMode ? i : indices[i];
    if (index < (UInt32)_db.SortedItems.Size())
    {
      int streamIndex = _db.Items[_db.SortedItems[index]].StreamIndex;
      if (streamIndex >= 0)
      {
        const CStreamInfo &si = _db.Streams[streamIndex];
        totalSize += si.Resource.UnpackSize;
      }
    }
    else
      totalSize += _xmls[index - (UInt32)_db.SortedItems.Size()].Data.GetCapacity();
  }

  RINOK(extractCallback->SetTotal(totalSize));

  UInt64 currentTotalPacked = 0;
  UInt64 currentTotalUnPacked = 0;
  UInt64 currentItemUnPacked, currentItemPacked;
  
  int prevSuccessStreamIndex = -1;

  CUnpacker unpacker;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (i = 0; i < numItems; currentTotalUnPacked += currentItemUnPacked,
      currentTotalPacked += currentItemPacked)
  {
    currentItemUnPacked = 0;
    currentItemPacked = 0;

    lps->InSize = currentTotalPacked;
    lps->OutSize = currentTotalUnPacked;

    RINOK(lps->SetCur());
    UInt32 index = allFilesMode ? i : indices[i];
    i++;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;

    CMyComPtr<ISequentialOutStream> realOutStream;
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    if (index >= (UInt32)_db.SortedItems.Size())
    {
      if (!testMode && !realOutStream)
        continue;
      RINOK(extractCallback->PrepareOperation(askMode));
      const CByteBuffer &data = _xmls[index - (UInt32)_db.SortedItems.Size()].Data;
      currentItemUnPacked = data.GetCapacity();
      if (realOutStream)
      {
        RINOK(WriteStream(realOutStream, (const Byte *)data, data.GetCapacity()));
        realOutStream.Release();
      }
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }

    const CItem &item = _db.Items[_db.SortedItems[index]];
    int streamIndex = item.StreamIndex;
    if (streamIndex < 0)
    {
      if (!testMode && !realOutStream)
        continue;
      RINOK(extractCallback->PrepareOperation(askMode));
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(item.HasStream() ?
            NExtract::NOperationResult::kDataError :
            NExtract::NOperationResult::kOK));
      continue;
    }

    const CStreamInfo &si = _db.Streams[streamIndex];
    currentItemUnPacked = si.Resource.UnpackSize;
    currentItemPacked = si.Resource.PackSize;

    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    Int32 opRes = NExtract::NOperationResult::kOK;
    if (streamIndex != prevSuccessStreamIndex || realOutStream)
    {
      Byte digest[20];
      const CVolume &vol = _volumes[si.PartNumber];
      HRESULT res = unpacker.Unpack(vol.Stream, si.Resource, vol.Header.IsLzxMode(),
          realOutStream, progress, digest);
      if (res == S_OK)
      {
        if (memcmp(digest, si.Hash, kHashSize) == 0)
          prevSuccessStreamIndex = streamIndex;
        else
          opRes = NExtract::NOperationResult::kCRCError;
      }
      else if (res == S_FALSE)
        opRes = NExtract::NOperationResult::kDataError;
      else
        return res;
    }
    realOutStream.Release();
    RINOK(extractCallback->SetOperationResult(opRes));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _db.SortedItems.Size();
  if (!_xmlInComments)
    *numItems += _xmls.Size();
  return S_OK;
}

}}
