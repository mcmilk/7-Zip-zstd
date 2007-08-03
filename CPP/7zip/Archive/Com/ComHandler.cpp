// ComHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "../../Common/StreamUtils.h"
#include "ComHandler.h"

namespace NArchive {
namespace NCom {

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  // { NULL, kpidAttributes, VT_UI4},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidCreationTime, VT_FILETIME},
  { NULL, kpidLastWriteTime, VT_FILETIME}
};

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
  const CRef &ref = _db.Refs[index];
  const CItem &item = _db.Items[ref.Did];
    
  switch(propID)
  {
    case kpidPath:
    {
      UString name = _db.GetItemPath(index);
      propVariant = name;
      break;
    }
    case kpidIsFolder:
      propVariant = item.IsDir();
      break;
    case kpidCreationTime:
      propVariant = item.CreationTime;
      break;
    case kpidLastWriteTime:
      propVariant = item.LastWriteTime;
      break;
    /*
    case kpidAttributes:
      propVariant = item.Falgs;
      break;
    */
    case kpidPackedSize:
      if (!item.IsDir())
      {
        int numBits = _db.IsLargeStream(item.Size) ? 
            _db.SectorSizeBits : 
            _db.MiniSectorSizeBits;
        propVariant = (item.Size + ((UInt32)1 << numBits) - 1) >> numBits << numBits;
        break;
      }
    case kpidSize:
      if (!item.IsDir())
        propVariant = (UInt64)item.Size;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  Close();
  try
  {
    if (OpenArchive(inStream, _db) != S_OK)
      return S_FALSE;
    _stream = inStream;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _db.Clear();
  _stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool testMode = (_aTestMode != 0);
  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = _db.Refs.Size();
  if (numItems == 0)
    return S_OK;
  UInt32 i;
  UInt64 totalSize = 0;
  for(i = 0; i < numItems; i++)
  {
    const CItem &item = _db.Items[_db.Refs[allFilesMode ? i : indices[i]].Did];
    if (!item.IsDir())
      totalSize += item.Size;
  }
  RINOK(extractCallback->SetTotal(totalSize));

  UInt64 currentTotalSize = 0, currentItemSize = 0;
  
  CByteBuffer sect;
  sect.SetCapacity((UInt32)1 << _db.SectorSizeBits);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    RINOK(extractCallback->SetCompleted(&currentTotalSize));
    Int32 index = allFilesMode ? i : indices[i];
    const CItem &item = _db.Items[_db.Refs[index].Did];
    currentItemSize = 0;
    if (!item.IsDir())
      currentItemSize = item.Size;

    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ? 
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }
    if (!testMode && (!realOutStream))
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    Int32 res = NArchive::NExtract::NOperationResult::kDataError;
    {
      UInt32 sid = item.Sid;
      UInt64 prev = 0;
      for (UInt64 pos = 0;;)
      {
        if (sid == NFatID::kEndOfChain)
        {
          if (pos != item.Size)
            break;
          res = NArchive::NExtract::NOperationResult::kOK;
          break;
        }
        if (pos >= item.Size)
          break;
          
        UInt64 offset;
        UInt32 size;

        if (_db.IsLargeStream(item.Size))
        {
          if (pos - prev > (1 << 20))
          {
            UInt64 processed = currentTotalSize + pos;
            RINOK(extractCallback->SetCompleted(&processed));
            prev = pos;
          }
          size = 1 << _db.SectorSizeBits;
          offset = ((UInt64)sid + 1) << _db.SectorSizeBits;
          if (sid >= _db.FatSize)
            break;
          sid = _db.Fat[sid];
        }
        else
        {
          int subBits = (_db.SectorSizeBits - _db.MiniSectorSizeBits);
          UInt32 fid = sid >> subBits;
          if (fid >= _db.NumSectorsInMiniStream)
            break;
          size = 1 << _db.MiniSectorSizeBits;
          offset = (((UInt64)_db.MiniSids[fid] + 1) << _db.SectorSizeBits) + 
            ((sid & ((1 << subBits) - 1)) << _db.MiniSectorSizeBits);
          if (sid >= _db.MatSize)
            break;
          sid = _db.Mat[sid];
        }
        
        // last sector can be smaller than sector size (it can contain requied data only).
        UInt64 rem = item.Size - pos;
        if (size > rem)
          size = (UInt32)rem;

        RINOK(_stream->Seek(offset, STREAM_SEEK_SET, NULL));
        UInt32 realProcessedSize;
        RINOK(ReadStream(_stream, sect, size, &realProcessedSize));
        if (realProcessedSize != size)
          break;

        if (realOutStream)
        {
          RINOK(WriteStream(realOutStream, sect, size, &realProcessedSize));
          if (realProcessedSize != size)
            break;
        }
        pos += size;
      }
    }
    realOutStream.Release();
    RINOK(extractCallback->SetOperationResult(res));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _db.Refs.Size();
  return S_OK;
}

}}
