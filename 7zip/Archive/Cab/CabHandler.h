// CabHandler.h

#pragma once

#ifndef __CAB_HANDLER_H
#define __CAB_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "CabIn.h"

namespace NArchive {
namespace NCab {

class CHandler: 
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Open)(IInStream *stream, 
      const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback);
  
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UINT32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

private:
  CObjectVector<NHeader::CFolder> m_Folders;
  CObjectVector<CItem> m_Files;
  CInArchiveInfo m_ArchiveInfo;
  CMyComPtr<IInStream> m_Stream;
};

}}

#endif