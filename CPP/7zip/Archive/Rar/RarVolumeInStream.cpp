// RarVolumeInStream.cpp

#include "StdAfx.h"

#include "../../../../C/7zCrc.h"

#include "RarVolumeInStream.h"

namespace NArchive {
namespace NRar {

void CFolderInStream::Init(
    CObjectVector<CInArchive> *archives,
    const CObjectVector<CItemEx> *items,
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
    const CItemEx &item = (*_items)[_refItem.ItemIndex + _curIndex];
    _stream.Attach((*_archives)[_refItem.VolumeIndex + _curIndex].
        CreateLimitedStream(item.GetDataPosition(), item.PackSize));
    _curIndex++;
    _fileIsOpen = true;
    _crc = CRC_INIT_VAL;
    return S_OK;
  }
  return S_OK;
}

HRESULT CFolderInStream::CloseStream()
{
  CRCs.Add(CRC_GET_DIGEST(_crc));
  _stream.Release();
  _fileIsOpen = false;
  return S_OK;
}

STDMETHODIMP CFolderInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize = 0;
  while ((_curIndex < _refItem.NumItems || _fileIsOpen) && size > 0)
  {
    if (_fileIsOpen)
    {
      UInt32 localProcessedSize;
      RINOK(_stream->Read(
          ((Byte *)data) + realProcessedSize, size, &localProcessedSize));
      _crc = CrcUpdate(_crc, ((Byte *)data) + realProcessedSize, localProcessedSize);
      if (localProcessedSize == 0)
      {
        RINOK(CloseStream());
        continue;
      }
      realProcessedSize += localProcessedSize;
      size -= localProcessedSize;
      break;
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

}}
