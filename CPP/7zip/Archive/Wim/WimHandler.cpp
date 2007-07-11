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

#ifdef WIM_DETAILS

enum 
{
  kpidVolume = kpidUserDefined,
  kpidOffset,
  kpidLinks
};

#endif

STATPROPSTG kProperties[] = 
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
  , { L"Volume", kpidVolume, VT_UI4}
  , { L"Offset", kpidOffset, VT_UI8}
  , { L"Links", kpidLinks, VT_UI4}
  #endif
};

static const wchar_t *kStreamsNamePrefix = L"Files" WSTRING_PATH_SEPARATOR;
static const wchar_t *kMethodLZX = L"LZX";
static const wchar_t *kMethodCopy = L"Copy";

STDMETHODIMP CHandler::GetArchiveProperty(PROPID /* propID */, PROPVARIANT *value)
{
  value->vt = VT_EMPTY;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfProperties(UInt32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CHandler::GetPropertyInfo(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  if(index >= sizeof(kProperties) / sizeof(kProperties[0]))
    return E_INVALIDARG;
  const STATPROPSTG &srcItem = kProperties[index];
  *propID = srcItem.propid;
  *varType = srcItem.vt;
  if (srcItem.lpwstrName == 0)
    *name = 0;
  else
    *name = ::SysAllocString(srcItem.lpwstrName);
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfArchiveProperties(UInt32 *numProperties)
{
  *numProperties = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetArchivePropertyInfo(UInt32 /* index */,     
      BSTR * /* name */, PROPID * /* propID */, VARTYPE * /* varType */)
{
  return E_INVALIDARG;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
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
          propVariant = item.Name;
        else
        {
          wchar_t sz[32];
          ConvertUInt64ToString(item.StreamIndex, sz);
          UString s = sz;
          while (s.Length() < m_NameLenForStreams)
            s = L'0' + s;
          s = UString(kStreamsNamePrefix) + s;
          propVariant = s;
          break;
        }
        break;
      case kpidIsFolder:
        propVariant = item.IsDirectory();
        break;
      case kpidAttributes:
        if (item.HasMetadata)
          propVariant = item.Attributes;
        break;
      case kpidCreationTime:
        if (item.HasMetadata)
          propVariant = item.CreationTime;
        break;
      case kpidLastAccessTime:
        if (item.HasMetadata)
          propVariant = item.LastAccessTime;
        break;
      case kpidLastWriteTime:
        if (item.HasMetadata)
          propVariant = item.LastWriteTime;
        break;
      case kpidPackedSize:
        if (si)
          propVariant = si->Resource.PackSize;
        else
          propVariant = (UInt64)0;
        break;
      case kpidSize:
        if (si)
          propVariant = si->Resource.UnpackSize;
        else
          propVariant = (UInt64)0;
        break;
      case kpidMethod:
        if (si)
          if (si->Resource.IsCompressed())
            propVariant = kMethodLZX;
          else
            propVariant = kMethodCopy;
        break;
      #ifdef WIM_DETAILS
      case kpidVolume:
        if (si)
          propVariant = (UInt32)si->PartNumber;
        break;
      case kpidOffset:
        if (si)
          propVariant = (UInt64)si->Resource.Offset;
        break;
      case kpidLinks:
        if (si)
          propVariant = (UInt32)si->RefCount;
        else
          propVariant = (UInt64)0;
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
          propVariant = s;
          break;
        }
        case kpidIsFolder:
          propVariant = false;
          break;
        case kpidPackedSize:
        case kpidSize:
          propVariant = (UInt64)m_Xmls[index].Data.GetCapacity();
          break;
        case kpidMethod:
          propVariant = L"Copy";
          break;
      }
    }
  }
  propVariant.Detach(value);
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
          NCOM::CPropVariant propVariant;
          RINOK(openVolumeCallback->GetProperty(kpidName, &propVariant));
          if (propVariant.vt != VT_BSTR)
            break;
          seqName.InitName(propVariant.bstrVal);
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

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize = 0;

  int prevSuccessStreamIndex = -1;

  CUnpacker unpacker;

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
  localProgressSpec->Init(extractCallback, false);
  
  CLocalCompressProgressInfo *localCompressProgressSpec = new CLocalCompressProgressInfo;
  CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

  for (i = 0; i < numItems; currentTotalSize += currentItemSize)
  {
    currentItemSize = 0;
    RINOK(extractCallback->SetCompleted(&currentTotalSize));
    UInt32 index = allFilesMode ? i : indices[i];
    i++;
    Int32 askMode = testMode ? 
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;

    CMyComPtr<ISequentialOutStream> realOutStream;
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    if (index >= (UInt32)m_Database.Items.Size())
    {
      if(!testMode && (!realOutStream))
        continue;
      RINOK(extractCallback->PrepareOperation(askMode));
      const CByteBuffer &data = m_Xmls[index - (UInt32)m_Database.Items.Size()].Data;
      currentItemSize = data.GetCapacity();
      if (realOutStream)
      {
        RINOK(WriteStream(realOutStream, (const Byte *)data, (UInt32)data.GetCapacity(), NULL));
        realOutStream.Release();
      }
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
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
            NArchive::NExtract::NOperationResult::kDataError :
            NArchive::NExtract::NOperationResult::kOK));
      continue;
    }

    const CStreamInfo &si = m_Database.Streams[streamIndex];
    currentItemSize = si.Resource.UnpackSize;

    if(!testMode && (!realOutStream))
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    Int32 opRes = NArchive::NExtract::NOperationResult::kOK;
    if (streamIndex != prevSuccessStreamIndex || realOutStream)
    {
      Byte digest[20];
      localCompressProgressSpec->Init(progress, &currentTotalSize, &currentTotalSize);
      HRESULT res = unpacker.Unpack(m_Volumes[si.PartNumber].Stream, si.Resource, realOutStream, compressProgress, digest);
      if (res == S_OK)
      {
        if (memcmp(digest, si.Hash, kHashSize) == 0)
          prevSuccessStreamIndex = streamIndex;
        else
          opRes = NArchive::NExtract::NOperationResult::kCRCError;
      }
      else if (res == S_FALSE)
        opRes = NArchive::NExtract::NOperationResult::kDataError;
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
