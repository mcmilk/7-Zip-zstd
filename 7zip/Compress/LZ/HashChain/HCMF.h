// HCMF.h

// #ifndef __HCMF_H
// #define __HCMF_H

#include "HC.h"

namespace HC_NAMESPACE {

#undef kIDByte
#undef kIDString

#ifdef HASH_ARRAY_2
  #ifdef HASH_ARRAY_3
    #ifdef HASH_BIG
      #define kIDByte 0x4
      #define kIDString TEXT("4b")
    #else
      #define kIDByte 0x3
      #define kIDString TEXT("4")
    #endif
  #else
    #define kIDByte 0x2
    #define kIDString TEXT("3")
  #endif
#else
  #ifdef HASH_ZIP
    #define kIDByte 0x0
    #define kIDString TEXT("3Z")
  #else
    #define kIDByte 0x1
    #define kIDString TEXT("2")
  #endif
#endif

#undef kIDUse3BytesByte
#undef kIDUse3BytesString

#define kIDUse3BytesByte 0x00
#define kIDUse3BytesString TEXT("")

// #undef kIDStringFull

// #define kIDStringFull TEXT("Compress.MatchFinderHC") kIDString kIDUse3BytesString

// {23170F69-40C1-278C-03XX-0000000000}
DEFINE_GUID(HC_CLSID, 
0x23170F69, 0x40C1, 0x278C, 0x03, kIDByte | kIDUse3BytesByte, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

class CInTree2: public CInTree
{
  CMyComPtr<IMatchFinderCallback> m_Callback;
  virtual void BeforeMoveBlock();
  virtual void AfterMoveBlock();
public:
  void SetCallback(IMatchFinderCallback *callback) { m_Callback = callback; }
};

class CMatchFinderHC: 
  public IMatchFinder,
  public IMatchFinderSetCallback,
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP1(IMatchFinderSetCallback)

  STDMETHOD(Init)(ISequentialInStream *inStream);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(MovePos)();
  STDMETHOD_(Byte, GetIndexByte)(Int32 index);
  STDMETHOD_(UInt32, GetMatchLen)(Int32 index, UInt32 back, UInt32 limit);
  STDMETHOD_(UInt32, GetNumAvailableBytes)();
  STDMETHOD_(const Byte *, GetPointerToCurrentPos)();
  STDMETHOD(Create)(UInt32 historySize, UInt32 keepAddBufferBefore, 
      UInt32 matchMaxLen, UInt32 keepAddBufferAfter);
  STDMETHOD_(UInt32, GetLongestMatch)(UInt32 *distances);
  STDMETHOD_(void, DummyLongestMatch)();

  // IMatchFinderSetCallback
  STDMETHOD(SetCallback)(IMatchFinderCallback *aCallback);

private:
  // UInt32 m_WindowReservSize;
  CInTree2 m_MatchFinder;
public:
  // CMatchFinderHC(): m_WindowReservSize((1 << 19) + 256) {};
  void SetCutValue(UInt32 cutValue) 
    { m_MatchFinder.SetCutValue(cutValue); }
  /*
  void SetWindowReservSize(UInt32 aReservWindowSize)
    { m_WindowReservSize = aReservWindowSize; }
  */
  virtual ~CMatchFinderHC() {}
};
 
}

// #endif

