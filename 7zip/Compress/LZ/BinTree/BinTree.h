// BinTree.h

// #ifndef __BINTREE_H
// #define __BINTREE_H

#include "../LZInWindow.h"
 
namespace BT_NAMESPACE {

typedef UInt32 CIndex;
const UInt32 kMaxValForNormalize = (UInt32(1) << 31) - 1;

// #define HASH_ARRAY_2

// #ifdef HASH_ARRAY_2

// #define HASH_ARRAY_3

// #else

// #define HASH_ZIP

// #endif

struct CPair
{
  CIndex Left;
  CIndex Right;
};

/*
const int kNumBundleBits = 2;
const UInt32 kNumPairsInBundle = 1 << kNumBundleBits;
const UInt32 kBundleMask = kNumPairsInBundle - 1;
const UInt32 kNumBundleBytes = kNumPairsInBundle * sizeof(CPair);

struct CBundle
{
  CPair Pairs[kNumPairsInBundle];
  Byte Bytes[kNumBundleBytes];
};
*/

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
  
  // CBundle *_son;
  CPair *_son;

  UInt32 _cutValue;

  void NormalizeLinks(CIndex *items, UInt32 numItems, UInt32 subValue);
  void Normalize();
  void FreeMemory();

public:
  CInTree();
  ~CInTree();
  HRESULT Create(UInt32 sizeHistory, UInt32 keepAddBufferBefore, UInt32 matchMaxLen, 
      UInt32 keepAddBufferAfter, UInt32 sizeReserv = (1<<17));
	HRESULT Init(ISequentialInStream *stream);
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
