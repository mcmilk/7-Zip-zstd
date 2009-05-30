// ComHandler.h

#ifndef __ARCHIVE_COM_HANDLER_H
#define __ARCHIVE_COM_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "ComIn.h"

namespace NArchive {
namespace NCom {

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  CDatabase _db;
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

}}

#endif
