// Split/Handler.h

#ifndef __SPLIT_HANDLER_H
#define __SPLIT_HANDLER_H

#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "../IArchive.h"

namespace NArchive {
namespace NSplit {

class CHandler: 
  public IInArchive,
  public IInArchiveGetStream,
  // public IOutArchive, 
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)

  INTERFACE_IInArchive(;)

  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);  

private:
  UString _subName;
  UString _name;
  CObjectVector<CMyComPtr<IInStream> > _streams;
  CRecordVector<UInt64> _sizes;

  UInt64 _totalSize;
};

}}

#endif
