// WimHandler.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/Defs.h"
#include "Common/ComTry.h"

#include "Windows/PropVariant.h"

#include "../../Common/StreamUtils.h"
#include "../../Common/ProgressUtils.h"

#include "WimHandler.h"

using namespace NWindows;

namespace NArchive {
namespace NWim {

#define WIM_DETAILS

STATPROPSTG kProps[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidAttributes, VT_UI8},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidCreationTime, VT_FILETIME},
  { NULL, kpidLastAccessTime, VT_FILETIME},
  { NULL, kpidLastWriteTime, VT_FILETIME}
  
  #ifdef WIM_DETAILS
  , { NULL, kpidVolume, VT_UI4}
  , { NULL, kpidOffset, VT_UI8}
  , { NULL, kpidLinks, VT_UI4}
  #endif
};

STATPROPSTG kArcProps[] = 
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidIsVolume, VT_BOOL},
  { NULL, kpidVolume, VT_UI4},
  { NULL, kpidNumVolumes, VT_UI4}
};

static const wchar_t *kStreamsNamePrefix = L"Files" WSTRING_PATH_SEPARATOR;
static const wchar_t *kMethodLZX = L"LZX";
static const wchar_t *kMethodCopy = L"Copy";

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidSize: prop = m_Database.GetUnpackSize(); break;
    case kpidPackedSize: prop = m_Database.GetPackSize(); break;
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
          prop = m_Volumes[volIndex].Header.PartNumber;
      }
      break;
    case kpidNumVolumes: if (m_Volumes.Size() > 0) prop = (UInt32)(m_Volumes.Size() - 1);
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
    if (item.StreamIndex >= 0)
      si = &m_Database.Streams[item.StreamIndex];

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
      case kpidIsFolder:
        prop = item.IsDirectory();
        break;
      case kpidAttributes:
        if (item.HasMetadata)
          prop = item.Attributes;
        break;
      case kpidCreationTime:
        if (item.HasMetadata)
          prop = item.CreationTime;
        break;
      case kpidLastAccessTime:
        if (item.HasMetadata)
          prop = item.LastAccessTime;
        break;
      case kpidLastWriteTime:
        if (item.HasMetadata)
          prop = item.LastWriteTime;
        break;
      case kpidPackedSize:
        if (si)
          prop = si->Resource.PackSize;
        else
          prop = (UInt64)0;
        break;
      case kpidSize:
        if (si)
          prop = si->Resource.UnpackSize;
        else
          prop = (UInt64)0;
        break;
      case kpidMethod:
        if (si)
          if (si->Resource.IsCompressed())
            prop = kMethodLZX;
          else
            prop = kMethodCopy;
        break;
      #ifdef WIM_DETAILS
      case kpidVolume:
        if (si)
          prop = (UInt32)si->PartNumber;
        break;
      case kpidOffset:
        if (si)
          prop = (UInt64)si->Resource.Offset;
        break;
      case kpidLinks:
        if (si)
          prop = (UInt32)si->RefCount;
        else
          prop = (UInt64)0;
        break;
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
        case kpidIsFolder:
          prop = false;
          break;
        case kpidPackedSize:
        case kpidSize:
          prop = (UInt64)m_Xmls[index].Data.GetCapacity();
          break;
        case kpidMethod:
          prop = L"Copy";
          break;
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
        m_Xmls.Add(xml);
      
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
        RINOK(WriteStream(realOutStream, (const Byte *)data, (UInt32)data.GetCapacity(), NULL));
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
      HRESULT res = unpacker.Unpack(m_Volumes[si.PartNumber].Stream, si.Resource, realOutStream, progress, digest);
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
