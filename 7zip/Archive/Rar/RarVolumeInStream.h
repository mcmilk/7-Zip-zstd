// RarVolumeInStream.h

#pragma once

#ifndef __RAR_VOLUME_IN_STREAM_H
#define __RAR_VOLUME_IN_STREAM_H

#include "../../IStream.h"
#include "Common/CRC.h"
#include "RarIn.h"

namespace NArchive {
namespace NRar {

struct CRefItem
{
  int VolumeIndex;
  int ItemIndex;
  int NumItems;
};

class CFolderInStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);

private:
  CObjectVector<CInArchive> *_archives;
  const CObjectVector<CItemEx> *_items;
  CRefItem _refItem;
  int _curIndex;
  CCRC _crc;
  bool _fileIsOpen;
  CMyComPtr<ISequentialInStream> _stream;

  HRESULT OpenStream();
  HRESULT CloseStream();
public:
  void Init(CObjectVector<CInArchive> *archives, 
      const CObjectVector<CItemEx> *items,
      const CRefItem &refItem);

  CRecordVector<UINT32> CRCs;
};
  
}}

#endif
