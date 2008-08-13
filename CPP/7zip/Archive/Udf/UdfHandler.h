// Udf/Handler.h

#ifndef __UDF_HANDLER_H
#define __UDF_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"

#include "UdfIn.h"

namespace NArchive {
namespace NUdf {

struct CRef2
{
  int Vol;
  int Fs;
  int Ref;
};

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _inStream;
  CInArchive _archive;
  CRecordVector<CRef2> _refs2;
public:
  MY_UNKNOWN_IMP1(IInArchive)
  INTERFACE_IInArchive(;)
};

}}

#endif
  