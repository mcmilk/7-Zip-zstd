// RarVolumeInStream.h

#pragma once

#ifndef __RARVOLUMEINSTREAM_H
#define __RARVOLUMEINSTREAM_H

#include "Interface/IInOutStreams.h"
#include "Archive/Rar/InEngine.h"
#include "Common/CRC.h"

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
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CFolderInStream)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CFolderInStream)
DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);

private:
  CObjectVector<CInArchive> *_archives;
  const CObjectVector<CItemInfoEx> *_items;
   
  CRefItem _refItem;

  int _curIndex;

  CCRC _crc;


  bool _fileIsOpen;

  CComPtr<ISequentialInStream> _stream;

  HRESULT OpenStream();
  HRESULT CloseStream();
public:
  void Init(CObjectVector<CInArchive> *archives, 
      const CObjectVector<CItemInfoEx> *items,
      const CRefItem &refItem);

  CRecordVector<UINT32> CRCs;
};
  
}}

#endif
