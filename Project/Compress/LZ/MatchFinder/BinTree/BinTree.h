// BinTree.h

// #pragma once

// #ifndef __BINTREE_H
// #define __BINTREE_H

#include "Stream/WindowIn.h"
#include "Common/Types.h"
#include "Windows/Defs.h"
 
namespace BT_NAMESPACE {


// #define __USE_3_BYTES

#ifdef __USE_3_BYTES

#pragma pack(push, PragmaBinTree)
#pragma pack(push, 1)

struct CIndex
{
  BYTE Data[3];
  CIndex(){}
  CIndex(UINT32 value)
  { 
    Data[0] = value & 0xFF;
    Data[1] = (value >> 8) & 0xFF;
    Data[2] = (value >> 16) & 0xFF;
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

#pragma pack(push, PragmaBinTreePair)
#pragma pack(push, 1)

struct CPair
{
  CIndex Left;
  CIndex Right;
};

#pragma pack(pop)
#pragma pack(pop, PragmaBinTreePair)

class CInTree: public NStream::NWindow::CIn
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
  
  CPair *_son;

  UINT32 _cutValue;

  void NormalizeLinks(CIndex *array, UINT32 numItems, UINT32 subValue);
  void Normalize();
  void FreeMemory();

public:
  CInTree();
  ~CInTree();
  HRESULT Create(UINT32 sizeHistory, UINT32 keepAddBufferBefore, UINT32 matchMaxLen, 
      UINT32 keepAddBufferAfter, UINT32 sizeReserv = (1<<17));
	HRESULT Init(ISequentialInStream *stream);
  void SetCutValue(UINT32 cutValue) { _cutValue = cutValue; }
  UINT32 GetLongestMatch(UINT32 *distances);
  void DummyLongestMatch();
  HRESULT MovePos()
  {
    _cyclicBufferPos++;
    if (_cyclicBufferPos >= _cyclicBufferSize)
      _cyclicBufferPos = 0;
    RETURN_IF_NOT_S_OK(CIn::MovePos());
    if (_pos == kMaxValForNormalize)
      Normalize();
    return S_OK;
  }
};

}

// #endif