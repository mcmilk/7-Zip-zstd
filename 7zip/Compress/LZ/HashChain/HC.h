// HC.h

// #pragma once

// #ifndef __HC_H
// #define __HC_H

#include "../LZInWindow.h"
#include "Common/Types.h"
#include "Windows/Defs.h"
 
namespace HC_NAMESPACE {


// #define __USE_3_BYTES

#ifdef __USE_3_BYTES

#pragma pack(push, PragmaBinTree)
#pragma pack(push, 1)

struct CIndex
{
  BYTE Data[3];
  CIndex(){}
  CIndex(UINT32 aValue)
  { 
    Data[0] = aValue & 0xFF;
    Data[1] = (aValue >> 8) & 0xFF;
    Data[2] = (aValue >> 16) & 0xFF;
  }
  operator UINT32() const { return (*((const UINT32 *)Data)) & 0xFFFFFF; }
};
const UINT32 kMaxValForNormalize = CIndex(-1);

#pragma pack(pop)
#pragma pack(pop, PragmaBinTree)

#else

typedef UINT32 CIndex;
const UINT32 kMaxValForNormalize = (UINT32(1) << 31) - 1;

#endif



// #define HASH_ARRAY_2

// #ifdef HASH_ARRAY_2

// #define HASH_ARRAY_3

// #else

// #define HASH_ZIP

// #endif

class CInTree: public CLZInWindow
{
  UINT32 _cyclicBufferPos;
  UINT32 _cyclicBufferSize;

  UINT32 _historySize;
  UINT32 _matchMaxLen;

  CIndex *_hash;
  
  #ifdef HASH_ARRAY_2
  CIndex *_hash2;
  #ifdef HASH_ARRAY_3
  CIndex *_hash3;
  #endif
  #endif
  
  CIndex *_chain;

  UINT32 _cutValue;

  void NormalizeLinks(CIndex *anArray, UINT32 aNumItems, UINT32 aSubValue);
  void Normalize();
  void FreeMemory();

public:
  CInTree();
  ~CInTree();
  HRESULT Create(UINT32 aSizeHistory, UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter, UINT32 _dwSizeReserv = (1<<17));
	HRESULT Init(ISequentialInStream *aStream);
  void SetCutValue(UINT32 aCutValue) { _cutValue = aCutValue; }
  UINT32 GetLongestMatch(UINT32 *aDistances);
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