// WimHandler.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/Defs.h"
#include "Common/ComTry.h"
#include "Common/StringToInt.h"
#include "Common/UTFConvert.h"

#include "Windows/PropVariant.h"

#include "../../Common/StreamUtils.h"
#include "../../Common/ProgressUtils.h"

#include "../../../../C/CpuArch.h"

#include "WimHandler.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

using namespace NWindows;

namespace NArchive {
namespace NWim {

#define WIM_DETAILS

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidAttrib, VT_UI4},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME}
  
  #ifdef WIM_DETAILS
  , { NULL, kpidVolume, VT_UI4}
  , { NULL, kpidOffset, VT_UI8}
  , { NULL, kpidLinks, VT_UI4}
  #endif
};

STATPROPSTG kArcProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidComment, VT_FILETIME},
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

void ParseTime(const CXmlItem &item, bool &defined, FILETIME &ft, const AString &s)
{
  defined = false;
  int cTimeIndex = item.FindSubTag(s);
  if (cTimeIndex >= 0)
  {
    const CXmlItem &timeItem = item.SubItems[cTimeIndex];
    UInt32 high = 0, low = 0;
    if (ParseNumber32(timeItem.GetSubStringForTag("HIGHPART"), high) &&
      ParseNumber32(timeItem.GetSubStringForTag("LOWPART"), low))
    {
      defined = true;
      ft.dwHighDateTime = high;
      ft.dwLowDateTime = low;
    }
  }
}

void CImageInfo::Parse(const CXmlItem &item)
{
  ParseTime(item, CTimeDefined, CTime, "CREATIONTIME");
  ParseTime(item, MTimeDefined, MTime, "LASTMODIFICATIONTIME");
  NameDefined = ConvertUTF8ToUnicode(item.GetSubStringForTag("NAME"), Name);
  // IndexDefined = ParseNumber32(item.GetPropertyValue("INDEX"), Index);
}

void CXml::Parse()
{
  size_t size = Data.GetCapacity();
  if (size < 2 || (size & 1) != 0 || (size > 1 << 24))
    return;
  const Byte *p = Data;
  if (Get16(p) != 0xFEFF)
    return;
  UString s;
  {
    wchar_t *chars = s.GetBuffer((int)size / 2 + 1);
    for (size_t i = 2; i < size; i += 2)
      *chars++ = (wchar_t)Get16(p + i);
    *chars = 0;
    s.ReleaseBuffer();
  }

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

static const wchar_t *kStreamsNamePrefix = L"Files" WSTRING_PATH_SEPARATOR;
static const wchar_t *kMethodLZX = L"LZX";
static const wchar_t *kMethodXpress = L"XPress";
static const wchar_t *kMethodCopy = L"Copy";

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;

  const CImageInfo *image = NULL;
  if (m_Xmls.Size() == 1)
  {
    const CXml &xml = m_Xmls[0];
    if (xml.Images.Size() == 1)
      image = &xml.Images[0];
  }

  switch(propID)
  {
    case kpidSize: prop = m_Database.GetUnpackSize(); break;
    case kpidPackSize: prop = m_Database.GetPackSize(); break;
    
    case kpidCTime:
      if (m_Xmls.Size() == 1)
      {
        const CXml &xml = m_Xmls[0];
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
      if (m_Xmls.Size() == 1)
      {
        const CXml &xml = m_Xmls[0];
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

    case kpidComment: if (image != NULL && image->NameDefined) prop = image->Name; break;

    case kpidIsVolume:
      if (m_Xmls.Size() > 0)
      {
        UInt16 volIndex = m_Xmls[0].VolIndex;
        if (volIndex < m_Volumes.Size())
          prop = (m_Volumes[volIndex].Header.NumParts > 1);
      }
      break;
    case kpidVolume:
      if (m_Xmls.Size() > 0)
      {
        UInt16 volIndex = m_Xmls[0].VolIndex;
        if (volIndex < m_Volumes.Size())
          prop = (UInt32)m_Volumes[volIndex].Header.PartNumber;
      }
      break;
    case kpidNumVolumes: if (m_Volumes.Size() > 0) prop = (UInt32)(m_Volumes.Size() - 1); break;
    case kpidMethod:
    {
      bool lzx = false, xpress = false, copy = false;
      for (int i = 0; i < m_Xmls.Size(); i++)
      {
        const CVolume &vol = m_Volumes[m_Xmls[i].VolIndex];
        const CHeader &header = vol.Header;
        if (header.IsCompressed())
          if (header.IsLzxMode())
            lzx = true;
          else
            xpress = true;
        else
          copy = true;
      }
      UString res;
      if (lzx)
        res = kMethodLZX;
      if (xpress)
      {
        if (!res.IsEmpty())
          res += L' ';
        res += kMethodXpress;
      }
      if (copy)
      {
        if (!res.IsEmpty())
          res += L' ';
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
  if (index < (UInt32)m_Database.Items.Size())
  {
    const CItem &item = m_Database.Items[index];
    const CStreamInfo *si = NULL;
    const CVolume *vol = NULL;
    if (item.StreamIndex >= 0)
    {
      si = &m_Database.Streams[item.StreamIndex];
      vol = &m_Volumes[si->PartNumber];
    }

    switch(propID)
    {
      case kpidPath:
        if (item.HasMetadata)
          prop = item.Name;
        else
        {
          wchar_t sz[32];
          ConvertUInt64ToString(item.StreamIndex, sz);
          UString s = sz;
          while (s.Length() < m_NameLenForStreams)
            s = L'0' + s;
          s = UString(kStreamsNamePrefix) + s;
          prop = s;
          break;
        }
        break;
      case kpidIsDir: prop = item.isDir(); break;
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
    index -= m_Database.Items.Size();
    {
      switch(propID)
      {
        case kpidPath:
        {
          wchar_t sz[32];
          ConvertUInt64ToString(m_Xmls[index].VolIndex, sz);
          UString s = (UString)sz + L".xml";
          prop = s;
          break;
        }
        case kpidIsDir: prop = false; break;
        case kpidPackSize:
        case kpidSize: prop = (UInt64)m_Xmls[index].Data.GetCapacity(); break;
        case kpidMethod: prop = L"Copy"; break;
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
    wchar_t s[32];
    ConvertUInt64ToString((index), s);
    return _before + (UString)s + _after;
  }
};

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  try
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
      if (firstVolumeIndex >= 0)
        if (!header.AreFromOnArchive(m_Volumes[firstVolumeIndex].Header))
          break;
      if (m_Volumes.Size() > header.PartNumber && m_Volumes[header.PartNumber].Stream)
        break;
      CXml xml;
      xml.VolIndex = header.PartNumber;
      res = OpenArchive(curStream, header, xml.Data, m_Database);
      if (res != S_OK)
      {
        if (i == 1)
          return res;
        if (res == S_FALSE)
          continue;
        return res;
      }
      
      while (m_Volumes.Size() <= header.PartNumber)
        m_Volumes.Add(CVolume());
      CVolume &volume = m_Volumes[header.PartNumber];
      volume.Header = header;
      volume.Stream = curStream;
      
      firstVolumeIndex = header.PartNumber;
      
      bool needAddXml = true;
      if (m_Xmls.Size() != 0)
        if (xml.Data == m_Xmls[0].Data)
          needAddXml = false;
      if (needAddXml)
      {
        xml.Parse();
        m_Xmls.Add(xml);
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

    RINOK(SortDatabase(m_Database));

    wchar_t sz[32];
    ConvertUInt64ToString(m_Database.Streams.Size(), sz);
    m_NameLenForStreams = MyStringLen(sz);
  }
  catch(...)
  {
    return S_FALSE;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  m_Database.Clear();
  m_Volumes.Clear();
  m_Xmls.Clear();
  m_NameLenForStreams = 0;
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UInt32(-1));

  if (allFilesMode)
    numItems = m_Database.Items.Size() + m_Xmls.Size();
  if (numItems == 0)
    return S_OK;
  bool testMode = (_aTestMode != 0);

  UInt32 i;
  UInt64 totalSize = 0;
  for (i = 0; i < numItems; i++)
  {
    UInt32 index = allFilesMode ? i : indices[i];
    if (index < (UInt32)m_Database.Items.Size())
    {
      int streamIndex = m_Database.Items[index].StreamIndex;
      if (streamIndex >= 0)
      {
        const CStreamInfo &si = m_Database.Streams[streamIndex];
        totalSize += si.Resource.UnpackSize;
      }
    }
    else
      totalSize += m_Xmls[index - (UInt32)m_Database.Items.Size()].Data.GetCapacity();
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
    if (index >= (UInt32)m_Database.Items.Size())
    {
      if(!testMode && (!realOutStream))
        continue;
      RINOK(extractCallback->PrepareOperation(askMode));
      const CByteBuffer &data = m_Xmls[index - (UInt32)m_Database.Items.Size()].Data;
      currentItemUnPacked = data.GetCapacity();
      if (realOutStream)
      {
        RINOK(WriteStream(realOutStream, (const Byte *)data, data.GetCapacity()));
        realOutStream.Release();
      }
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }

    const CItem &item = m_Database.Items[index];
    int streamIndex = item.StreamIndex;
    if (streamIndex < 0)
    {
      if(!testMode && (!realOutStream))
        continue;
      RINOK(extractCallback->PrepareOperation(askMode));
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(item.HasStream() ?
            NExtract::NOperationResult::kDataError :
            NExtract::NOperationResult::kOK));
      continue;
    }

    const CStreamInfo &si = m_Database.Streams[streamIndex];
    currentItemUnPacked = si.Resource.UnpackSize;
    currentItemPacked = si.Resource.PackSize;

    if(!testMode && (!realOutStream))
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    Int32 opRes = NExtract::NOperationResult::kOK;
    if (streamIndex != prevSuccessStreamIndex || realOutStream)
    {
      Byte digest[20];
      const CVolume &vol = m_Volumes[si.PartNumber];
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
  *numItems = m_Database.Items.Size() + m_Xmls.Size();
  return S_OK;
}

}}
