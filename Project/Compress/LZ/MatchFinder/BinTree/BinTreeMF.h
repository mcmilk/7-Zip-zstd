// BinTreeMF.h

// #pragma once

// #ifndef __BINTREEMF_H
// #define __BINTREEMF_H

#include "../../../Interface/CompressInterface.h"
#include "BinTree.h"

namespace BT_NAMESPACE {

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

#ifdef __USE_3_BYTES
  #define kIDUse3BytesByte 0x80
  #define kIDUse3BytesString TEXT("T")
#else
  #define kIDUse3BytesByte 0x00
  #define kIDUse3BytesString TEXT("")
#endif

#undef kIDStringFull

#define kIDStringFull TEXT("Compress.MatchFinderBT") kIDString kIDUse3BytesString

// {23170F69-40C1-278C-02XX-0000000000}
DEFINE_GUID(BT_CLSID, 
0x23170F69, 0x40C1, 0x278C, 0x02, kIDByte | kIDUse3BytesByte, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

class CInTree2: public CInTree
{
  CComPtr<IMatchFinderCallback> m_Callback;
  virtual void BeforeMoveBlock();
  virtual void AfterMoveBlock();
public:
  void SetCallback(IMatchFinderCallback *aCallback)
  {
    m_Callback = aCallback;
  }
};

class CMatchFinderBinTree: 
  public IInWindowStreamMatch,
  public IMatchFinderSetCallback,
  public CComObjectRoot,
  public CComCoClass<CMatchFinderBinTree, &BT_CLSID>
{
BEGIN_COM_MAP(CMatchFinderBinTree)
  COM_INTERFACE_ENTRY(IInWindowStreamMatch)
  COM_INTERFACE_ENTRY(IMatchFinderSetCallback)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CMatchFinderBinTree)

DECLARE_REGISTRY(CMatchFinderBinTree, kIDStringFull TEXT(".1"), kIDStringFull, UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Init)(ISequentialInStream *aStream);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(MovePos)();
  STDMETHOD_(BYTE, GetIndexByte)(UINT32 anIndex);
  STDMETHOD_(UINT32, GetMatchLen)(UINT32 aIndex, UINT32 aBack, UINT32 aLimit);
  STDMETHOD_(UINT32, GetNumAvailableBytes)();
  STDMETHOD_(const BYTE *, GetPointerToCurrentPos)();
  STDMETHOD(Create)(UINT32 aSizeHistory, 
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter);
  STDMETHOD_(UINT32, GetLongestMatch)(UINT32 *aDistances);
  STDMETHOD_(void, DummyLongestMatch)();

  // IMatchFinderSetCallback
  STDMETHOD(SetCallback)(IMatchFinderCallback *aCallback);

private:
  // UINT32 m_WindowReservSize;
  CInTree2 m_MatchFinder;
public:
  // CMatchFinderBinTree(): m_WindowReservSize((1 << 19) + 256) {};
  void SetCutValue(UINT32 aCutValue) 
    { m_MatchFinder.SetCutValue(aCutValue); }
  /*
  void SetWindowReservSize(UINT32 aReservWindowSize)
    { m_WindowReservSize = aReservWindowSize; }
  */
};
 
}

// #endif

