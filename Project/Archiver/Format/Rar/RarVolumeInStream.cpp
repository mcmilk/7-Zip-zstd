// RarVolumeInStream.cpp

#include "StdAfx.h"

#include "RarVolumeInStream.h"

#include "Windows/Defs.h"
#include "Common/Defs.h"

namespace NArchive {
namespace NRar {

void CFolderInStream::Init(
    CObjectVector<CInArchive> *archives, 
    const CObjectVector<CItemInfoEx> *items,
    const CRefItem &refItem)
{
  _archives = archives;
  _items = items;
  _refItem = refItem;
  _curIndex = 0;
  CRCs.Clear();
  _fileIsOpen = false;
}

HRESULT CFolderInStream::OpenStream()
{
  while (_curIndex < _refItem.NumItems)
  {
    const CItemInfoEx &item = (*_items)[_refItem.ItemIndex + _curIndex];
    _stream.Attach((*_archives)[_refItem.VolumeIndex + _curIndex].
        CreateLimitedStream(item.GetDataPosition(), item.PackSize));
    _curIndex++;
    _fileIsOpen = true;
    _crc.Init();
    return S_OK;
  }
  return S_OK;
}

HRESULT CFolderInStream::CloseStream()
{
  CRCs.Add(_crc.GetDigest());
  _stream.Release();
  _fileIsOpen = false;
  return S_OK;
}

STDMETHODIMP CFolderInStream::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize = 0;
  while ((_curIndex < _refItem.NumItems || _fileIsOpen) && size > 0)
  {
    if (_fileIsOpen)
    {
      UINT32 localProcessedSize;
      RINOK(_stream->Read(
          ((BYTE *)data) + realProcessedSize, size, &localProcessedSize));
      _crc.Update(((BYTE *)data) + realProcessedSize, localProcessedSize);
      if (localProcessedSize == 0)
      {
        RINOK(CloseStream());
        continue;
      }
      realProcessedSize += localProcessedSize;
      size -= localProcessedSize;
    }
    else
    {
      RINOK(OpenStream());
    }
  }
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return S_OK;
}

STDMETHODIMP CFolderInStream::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  return Read(data, size, processedSize);
}

}}