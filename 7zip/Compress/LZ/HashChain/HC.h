// HC.h

// #ifndef __HC_H
// #define __HC_H

#include "../LZInWindow.h"
 
namespace HC_NAMESPACE {

typedef UInt32 CIndex;
const UInt32 kMaxValForNormalize = (UInt32(1) << 31) - 1;

// #define HASH_ARRAY_2

// #ifdef HASH_ARRAY_2

// #define HASH_ARRAY_3

// #else

// #define HASH_ZIP

// #endif

class CInTree: public CLZInWindow
{
  UInt32 _cyclicBufferPos;
  UInt32 _cyclicBufferSize;

  UInt32 _historySize;
  UInt32 _matchMaxLen;

  CIndex *_hash;
  
  #ifdef HASH_ARRAY_2
  CIndex *_hash2;
  #ifdef HASH_ARRAY_3
  CIndex *_hash3;
  #endif
  #endif
  
  CIndex *_chain;

  UInt32 _cutValue;

  void NormalizeLinks(CIndex *anArray, UInt32 aNumItems, UInt32 aSubValue);
  void Normalize();
  void FreeMemory();

public:
  CInTree();
  ~CInTree();
  HRESULT Create(UInt32 historySize, UInt32 keepAddBufferBefore, UInt32 matchMaxLen, 
      UInt32 keepAddBufferAfter, UInt32 _sizeReserv = (1<<17));
	HRESULT Init(ISequentialInStream *inStream);
  void SetCutValue(UInt32 cutValue) { _cutValue = cutValue; }
  UInt32 GetLongestMatch(UInt32 *distances);
  void DummyLongestMatch();
  HRESULT MovePos()
  {
    _cyclicBufferPos++;
    if (_cyclicBufferPos >= _cyclicBufferSize)
      _cyclicBufferPos = 0;
    RINOK(CLZInWindow::MovePos());
    if (_pos == kMaxValForNormalize)
      Normalize();
    return S_OK;
  }
};

}

// #endif
