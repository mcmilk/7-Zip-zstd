// HC.h

#include "../LZInWindow.h"
 
namespace HC_NAMESPACE {

typedef UInt32 CIndex;
const UInt32 kMaxValForNormalize = (UInt32(1) << 31) - 1;

class CInTree: public CLZInWindow
{
  UInt32 _cyclicBufferPos;
  UInt32 _cyclicBufferSize; // it must be historySize + 1
  UInt32 _matchMaxLen;

  CIndex *_hash;
  
  UInt32 _cutValue;

  void Normalize();
  void FreeThisClassMemory();
  void FreeMemory();

public:
  CInTree();
  ~CInTree();
  HRESULT Create(UInt32 historySize, UInt32 keepAddBufferBefore, UInt32 matchMaxLen, 
      UInt32 keepAddBufferAfter, UInt32 sizeReserv = (1 << 17));
	HRESULT Init(ISequentialInStream *inStream);
  void SetCutValue(UInt32 cutValue) { _cutValue = cutValue; }
  UInt32 GetLongestMatch(UInt32 *distances);
  void DummyLongestMatch();
  HRESULT MovePos()
  {
    if (++_cyclicBufferPos == _cyclicBufferSize)
      _cyclicBufferPos = 0;
    RINOK(CLZInWindow::MovePos());
    if (_pos == kMaxValForNormalize)
      Normalize();
    return S_OK;
  }
};

}
