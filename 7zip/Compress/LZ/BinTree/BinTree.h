// BinTree.h

#include "../LZInWindow.h"
#include "../IMatchFinder.h"
 
namespace BT_NAMESPACE {

typedef UInt32 CIndex;
const UInt32 kMaxValForNormalize = (UInt32(1) << 31) - 1;

class CMatchFinderBinTree: 
  public IMatchFinder,
  public IMatchFinderSetCallback,
  public CLZInWindow,
  public CMyUnknownImp
{
  UInt32 _cyclicBufferPos;
  UInt32 _cyclicBufferSize; // it must be historySize + 1
  UInt32 _matchMaxLen;
  CIndex *_hash;
  UInt32 _cutValue;

  CMyComPtr<IMatchFinderCallback> m_Callback;

  void Normalize();
  void FreeThisClassMemory();
  void FreeMemory();

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
  STDMETHOD(SetCallback)(IMatchFinderCallback *callback);

  virtual void BeforeMoveBlock();
  virtual void AfterMoveBlock();

public:
  CMatchFinderBinTree();
  virtual ~CMatchFinderBinTree();
  void SetCutValue(UInt32 cutValue) { _cutValue = cutValue; }
};

}
