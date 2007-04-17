// RarVolumeInStream.h

#ifndef __RAR_VOLUME_IN_STREAM_H
#define __RAR_VOLUME_IN_STREAM_H

#include "../../IStream.h"
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

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);

private:
  CObjectVector<CInArchive> *_archives;
  const CObjectVector<CItemEx> *_items;
  CRefItem _refItem;
  int _curIndex;
  UInt32 _crc;
  bool _fileIsOpen;
  CMyComPtr<ISequentialInStream> _stream;

  HRESULT OpenStream();
  HRESULT CloseStream();
public:
  void Init(CObjectVector<CInArchive> *archives, 
      const CObjectVector<CItemEx> *items,
      const CRefItem &refItem);

  CRecordVector<UInt32> CRCs;
};
  
}}

#endif
