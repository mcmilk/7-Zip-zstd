// Zip/Handler.h

#ifndef __ZIP_HANDLER_H
#define __ZIP_HANDLER_H

#include "Common/DynamicBuffer.h"
#include "../../ICoder.h"
#include "../IArchive.h"

#include "ZipIn.h"
#include "ZipCompressionMode.h"

#ifdef COMPRESS_MT
#include "../../../Windows/System.h"
#endif

namespace NArchive {
namespace NZip {

class CHandler: 
  public IInArchive,
  public IOutArchive,
  public ISetProperties,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP3(
      IInArchive,
      IOutArchive,
      ISetProperties
      )

  STDMETHOD(Open)(IInStream *aStream, 
      const UInt64 *aMaxCheckStartPosition,
      IArchiveOpenCallback *anOpenArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, 
      Int32 testMode, IArchiveExtractCallback *anExtractCallback);

  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  // IOutArchive
  STDMETHOD(UpdateItems)(ISequentialOutStream *outStream, UInt32 numItems,
      IArchiveUpdateCallback *updateCallback);
  STDMETHOD(GetFileTimeType)(UInt32 *timeType);  

  // ISetProperties
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties);

  CHandler();
private:
  CObjectVector<CItemEx> m_Items;
  CInArchive m_Archive;
  bool m_ArchiveIsOpen;

  int m_Level;
  int m_MainMethod;
  UInt32 m_DicSize;
  UInt32 m_NumPasses;
  UInt32 m_NumFastBytes;
  UInt32 m_NumMatchFinderCycles;
  bool m_NumMatchFinderCyclesDefined;

  bool m_IsAesMode;
  Byte m_AesKeyMode;

  #ifdef COMPRESS_MT
  UInt32 _numThreads;
  #endif

  void InitMethodProperties()
  {
    m_Level = -1;
    m_MainMethod = -1;
    m_DicSize = 
    m_NumPasses = 
    m_NumFastBytes = 
    m_NumMatchFinderCycles = 0xFFFFFFFF;
    m_NumMatchFinderCyclesDefined = false;
    m_IsAesMode = false;
    m_AesKeyMode = 3; // aes-256
    #ifdef COMPRESS_MT
    _numThreads = NWindows::NSystem::GetNumberOfProcessors();;
    #endif
  }
};

}}

#endif
