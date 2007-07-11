// WimHandler.h

#ifndef __ARCHIVE_WIM_HANDLER_H
#define __ARCHIVE_WIM_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "WimIn.h"

namespace NArchive {
namespace NWim {

struct CVolume
{
  CHeader Header;
  CMyComPtr<IInStream> Stream;
};

struct CXml
{
  CByteBuffer Data;
  UInt16 VolIndex;
};

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  STDMETHOD(Open)(IInStream *stream, 
      const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, 
      Int32 testMode, IArchiveExtractCallback *extractCallback);
  
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

private:
  CDatabase m_Database;
  CObjectVector<CVolume> m_Volumes;
  CObjectVector<CXml> m_Xmls;
  int m_NameLenForStreams;
};

}}

#endif
