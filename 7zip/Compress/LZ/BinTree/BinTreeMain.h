// BinTreemain.h

// #include "StdAfx.h"

// #include "BinTree.h"
// #include "Common/NewHandler.h"

#include "../../../../Common/Defs.h"
#include "../../../../Common/CRC.h"

namespace BT_NAMESPACE {

#ifdef HASH_ARRAY_2
  static const UINT32 kHash2Size = 1 << 10;
  #ifdef HASH_ARRAY_3
    static const UINT32 kNumHashDirectBytes = 0;
    static const UINT32 kNumHashBytes = 4;
    static const UINT32 kHash3Size = 1 << 18;
    #ifdef HASH_BIG
    static const UINT32 kHashSize = 1 << 23;
    #else
    static const UINT32 kHashSize = 1 << 20;
    #endif
  #else
    static const UINT32 kNumHashDirectBytes = 3;
    static const UINT32 kNumHashBytes = 3;
    static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#else
  #ifdef HASH_ZIP 
    static const UINT32 kNumHashDirectBytes = 0;
    static const UINT32 kNumHashBytes = 3;
    static const UINT32 kHashSize = 1 << 16;
  #else
    static const UINT32 kNumHashDirectBytes = 2;
    static const UINT32 kNumHashBytes = 2;
    static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#endif


CInTree::CInTree():
  #ifdef HASH_ARRAY_2
  _hash2(0),
  #ifdef HASH_ARRAY_3
  _hash3(0),
  #endif
  #endif
  _hash(0),
  _son(0),
  _cutValue(0xFF)
{
}

void CInTree::FreeMemory()
{
  #ifdef WIN32
  if (_son != 0)
    VirtualFree(_son, 0, MEM_RELEASE);
  if (_hash != 0)
    VirtualFree(_hash, 0, MEM_RELEASE);
  #else
  delete []_son;
  delete []_hash;
  #endif
  _son = 0;
  _hash = 0;
  CLZInWindow::Free();
}

CInTree::~CInTree()
{ 
  FreeMemory();
}

HRESULT CInTree::Create(UINT32 sizeHistory, UINT32 keepAddBufferBefore, 
    UINT32 matchMaxLen, UINT32 keepAddBufferAfter, UINT32 sizeReserv)
{
  FreeMemory();
  try
  {
    CLZInWindow::Create(sizeHistory + keepAddBufferBefore, 
      matchMaxLen + keepAddBufferAfter, sizeReserv);
    
    if (_blockSize + 256 > kMaxValForNormalize)
      return E_INVALIDARG;
    
    _historySize = sizeHistory;
    _matchMaxLen = matchMaxLen;

    _cyclicBufferSize = sizeHistory + 1;
    
    
    UINT32 size = kHashSize;
    #ifdef HASH_ARRAY_2
    size += kHash2Size;
    #ifdef HASH_ARRAY_3
    size += kHash3Size;
    #endif
    #endif
    
    #ifdef WIN32
    _son = (CPair *)::VirtualAlloc(0, (_cyclicBufferSize + 1) * sizeof(CPair), MEM_COMMIT, PAGE_READWRITE);
    if (_son == 0)
      throw 1; // CNewException();
    _hash = (CIndex *)::VirtualAlloc(0, (size + 1) * sizeof(CIndex), MEM_COMMIT, PAGE_READWRITE);
    if (_hash == 0)
      throw 1; // CNewException();
    #else
    _son = new CPair[_cyclicBufferSize + 1];
    _hash = new CIndex[size + 1];
    #endif
    
    // m_RightBase = &m_LeftBase[_blockSize];
    
    // _hash = &m_RightBase[_blockSize];
    #ifdef HASH_ARRAY_2
    _hash2 = &_hash[kHashSize]; 
    #ifdef HASH_ARRAY_3
    _hash3 = &_hash2[kHash2Size]; 
    #endif
    #endif
    return S_OK;
  }
  catch(...)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
}

static const UINT32 kEmptyHashValue = 0;

HRESULT CInTree::Init(ISequentialInStream *stream)
{
  RINOK(CLZInWindow::Init(stream));
  int i;
  for(i = 0; i < kHashSize; i++)
    _hash[i] = kEmptyHashValue;

  #ifdef HASH_ARRAY_2
  for(i = 0; i < kHash2Size; i++)
    _hash2[i] = kEmptyHashValue;
  #ifdef HASH_ARRAY_3
  for(i = 0; i < kHash3Size; i++)
    _hash3[i] = kEmptyHashValue;
  #endif
  #endif

  _cyclicBufferPos = 0;

  ReduceOffsets(0 - 1);
  return S_OK;
}


#ifdef HASH_ARRAY_2
#ifdef HASH_ARRAY_3
inline UINT32 Hash(const BYTE *pointer, UINT32 &hash2Value, UINT32 &hash3Value)
{
  UINT32 temp = CCRC::Table[pointer[0]] ^ pointer[1];
  hash2Value = temp & (kHash2Size - 1);
  hash3Value = (temp ^ (UINT32(pointer[2]) << 8)) & (kHash3Size - 1);
  return (temp ^ (UINT32(pointer[2]) << 8) ^ (CCRC::Table[pointer[3]] << 5)) & 
      (kHashSize - 1);
}
#else // no HASH_ARRAY_3
inline UINT32 Hash(const BYTE *pointer, UINT32 &hash2Value)
{
  hash2Value = (CCRC::Table[pointer[0]] ^ pointer[1]) & (kHash2Size - 1);
  return (*((const UINT32 *)pointer)) & 0xFFFFFF;
}
#endif // HASH_ARRAY_3
#else // no HASH_ARRAY_2
#ifdef HASH_ZIP 
inline UINT32 Hash(const BYTE *pointer)
{
  return ((UINT32(pointer[0]) << 8) ^ 
      CCRC::Table[pointer[1]] ^ pointer[2]) & (kHashSize - 1);
}
#else // no HASH_ZIP 
inline UINT32 Hash(const BYTE *pointer)
{
  return pointer[0] ^ (UINT32(pointer[1]) << 8);
}
#endif // HASH_ZIP
#endif // HASH_ARRAY_2

UINT32 CInTree::GetLongestMatch(UINT32 *distances)
{
  UINT32 currentLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    currentLimit = _matchMaxLen;
  else
  {
    currentLimit = _streamPos - _pos;
    if(currentLimit < kNumHashBytes)
      return 0; 
  }

  UINT32 matchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  BYTE *cur = _buffer + _pos;
  
  UINT32 matchHashLenMax = 0;

  #ifdef HASH_ARRAY_2
  UINT32 hash2Value;
  #ifdef HASH_ARRAY_3
  UINT32 hash3Value;
  UINT32 hashValue = Hash(cur, hash2Value, hash3Value);
  #else
  UINT32 hashValue = Hash(cur, hash2Value);
  #endif
  #else
  UINT32 hashValue = Hash(cur);
  #endif

  UINT32 curMatch = _hash[hashValue];
  #ifdef HASH_ARRAY_2
  UINT32 curMatch2 = _hash2[hash2Value];
  #ifdef HASH_ARRAY_3
  UINT32 curMatch3 = _hash3[hash3Value];
  #endif
  _hash2[hash2Value] = _pos;
  bool matchLen2Exist = false;
  UINT32 len2Distance = 0;
  if(curMatch2 >= matchMinPos)
  {
    if (_buffer[curMatch2] == cur[0])
    {
      len2Distance = _pos - curMatch2 - 1;
      matchHashLenMax = 2;
      matchLen2Exist = true;
    }
  }

  #ifdef HASH_ARRAY_3
  _hash3[hash3Value] = _pos;
  UINT32 matchLen3Exist = false;
  UINT32 len3Distance = 0;
  if(curMatch3 >= matchMinPos)
  {
    if (_buffer[curMatch3] == cur[0])
    {
      len3Distance = _pos - curMatch3 - 1;
      matchHashLenMax = 3;
      matchLen3Exist = true;
      if (matchLen2Exist)
      {
        if (len3Distance < len2Distance)
          len2Distance = len3Distance;
      }
      else
      {
        len2Distance = len3Distance;
        matchLen2Exist = true;
      }
    }
  }
  #endif
  #endif

  _hash[hashValue] = _pos;

  if(curMatch < matchMinPos)
  {
    _son[_cyclicBufferPos].Left = kEmptyHashValue; 
    _son[_cyclicBufferPos].Right = kEmptyHashValue; 

    #ifdef HASH_ARRAY_2
    distances[2] = len2Distance;
    #ifdef HASH_ARRAY_3
    distances[3] = len3Distance;
    #endif
    #endif

    return matchHashLenMax;
  }
  CIndex *ptrLeft = &_son[_cyclicBufferPos].Right;
  CIndex *ptrRight = &_son[_cyclicBufferPos].Left;

  UINT32 maxLen, minSameLeft, minSameRight, minSame;
  maxLen = minSameLeft = minSameRight = minSame = kNumHashDirectBytes;

  #ifdef HASH_ARRAY_2
  #ifndef HASH_ARRAY_3
    if (matchLen2Exist)
      distances[2] = len2Distance;
    else
      if (kNumHashDirectBytes >= 2)
        distances[2] = _pos - curMatch - 1;
  #endif
  #endif

  distances[maxLen] = _pos - curMatch - 1;
  
  for(UINT32 count = _cutValue; count > 0; count--)
  {
    BYTE *pby1 = _buffer + curMatch;
    // CIndex left = _son[curMatch].Left; // it's prefetch
    UINT32 currentLen;
    for(currentLen = minSame; currentLen < currentLimit; currentLen++/*, dwComps++*/)
      if (pby1[currentLen] != cur[currentLen])
        break;
    while (currentLen > maxLen)
      distances[++maxLen] = _pos - curMatch - 1;
    
    UINT32 delta = _pos - curMatch;
    UINT32 cyclicPos = (delta <= _cyclicBufferPos) ?
        (_cyclicBufferPos - delta):
        (_cyclicBufferPos - delta + _cyclicBufferSize);

    if (currentLen != currentLimit)
    {
      if (pby1[currentLen] < cur[currentLen])
      {
        *ptrRight = curMatch;
        ptrRight = &_son[cyclicPos].Right;
        curMatch = _son[cyclicPos].Right;
        if(currentLen > minSameLeft)
        {
          minSameLeft = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
      else
      {
        *ptrLeft = curMatch;
        ptrLeft = &_son[cyclicPos].Left;
        // curMatch = left;
        curMatch = _son[cyclicPos].Left;
        if(currentLen > minSameRight)
        {
          minSameRight = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
    }
    else
    {
      if(currentLen < _matchMaxLen)
      {
        *ptrLeft = curMatch;
        ptrLeft = &_son[cyclicPos].Left;
        curMatch = _son[cyclicPos].Left;
        if(currentLen > minSameRight)
        {
          minSameRight = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
      else
      {
        *ptrLeft = _son[cyclicPos].Right;
        *ptrRight = _son[cyclicPos].Left;

        #ifdef HASH_ARRAY_2
        if (matchLen2Exist && len2Distance < distances[2])
          distances[2] = len2Distance;
        #ifdef HASH_ARRAY_3
        if (matchLen3Exist && len3Distance < distances[3])
          distances[3] = len3Distance;
        #endif
        #endif

        return maxLen;
      }
    }
    if(curMatch < matchMinPos)
      break;
  }
  *ptrLeft = kEmptyHashValue;
  *ptrRight = kEmptyHashValue;
  #ifdef HASH_ARRAY_2
  if (matchLen2Exist)
  {
    if (maxLen < 2)
    {
      distances[2] = len2Distance;
      maxLen = 2;
    }
    else if (len2Distance < distances[2])
      distances[2] = len2Distance;
  }
  #ifdef HASH_ARRAY_3
  if (matchLen3Exist)
  {
    if (maxLen < 3)
    {
      distances[3] = len3Distance;
      maxLen = 3;
    }
    else if (len3Distance < distances[3])
      distances[3] = len3Distance;
  }
  #endif
  #endif
  return maxLen;
}

void CInTree::DummyLongestMatch()
{
  UINT32 currentLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    currentLimit = _matchMaxLen;
  else
  {
    currentLimit = _streamPos - _pos;
    if(currentLimit < kNumHashBytes)
      return; 
  }
  UINT32 matchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  BYTE *cur = _buffer + _pos;
  
  #ifdef HASH_ARRAY_2
  UINT32 hash2Value;
  #ifdef HASH_ARRAY_3
  UINT32 hash3Value;
  UINT32 hashValue = Hash(cur, hash2Value, hash3Value);
  _hash3[hash3Value] = _pos;
  #else
  UINT32 hashValue = Hash(cur, hash2Value);
  #endif
  _hash2[hash2Value] = _pos;
  #else
  UINT32 hashValue = Hash(cur);
  #endif

  UINT32 curMatch = _hash[hashValue];
  _hash[hashValue] = _pos;

  if(curMatch < matchMinPos)
  {
    _son[_cyclicBufferPos].Left = kEmptyHashValue; 
    _son[_cyclicBufferPos].Right = kEmptyHashValue; 
    return;
  }
  CIndex *ptrLeft = &_son[_cyclicBufferPos].Right;
  CIndex *ptrRight = &_son[_cyclicBufferPos].Left;

  UINT32 maxLen, minSameLeft, minSameRight, minSame;
  maxLen = minSameLeft = minSameRight = minSame = kNumHashDirectBytes;
  for(UINT32 count = _cutValue; count > 0; count--)
  {
    BYTE *pby1 = _buffer + curMatch;
    // CIndex left = _son[curMatch].Left; // it's prefetch
    UINT32 currentLen;
    for(currentLen = minSame; currentLen < currentLimit; currentLen++/*, dwComps++*/)
      if (pby1[currentLen] != cur[currentLen])
        break;

    UINT32 delta = _pos - curMatch;
    UINT32 cyclicPos = (delta <= _cyclicBufferPos) ?
        (_cyclicBufferPos - delta):
        (_cyclicBufferPos - delta + _cyclicBufferSize);
    
    if (currentLen != currentLimit)
    {
      if (pby1[currentLen] < cur[currentLen])
      {
        *ptrRight = curMatch;
        ptrRight = &_son[cyclicPos].Right;
        curMatch = _son[cyclicPos].Right;
        if(currentLen > minSameLeft)
        {
          minSameLeft = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
      else 
      {
        *ptrLeft = curMatch;
        ptrLeft = &_son[cyclicPos].Left;
        curMatch = _son[cyclicPos].Left;
        // curMatch = left;
        if(currentLen > minSameRight)
        {
          minSameRight = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
    }
    else
    {
      if(currentLen < _matchMaxLen)
      {
        *ptrLeft = curMatch;
        ptrLeft = &_son[cyclicPos].Left;
        curMatch = _son[cyclicPos].Left;
        if(currentLen > minSameRight)
        {
          minSameRight = currentLen;
          minSame = MyMin(minSameLeft, minSameRight);
        }
      }
      else
      {
        *ptrLeft = _son[cyclicPos].Right;
        *ptrRight = _son[cyclicPos].Left;
        return;
      }
    }
    if(curMatch < matchMinPos)
      break;
  }
  *ptrLeft = kEmptyHashValue;
  *ptrRight = kEmptyHashValue;
}

void CInTree::NormalizeLinks(CIndex *array, UINT32 numItems, UINT32 subValue)
{
  for (UINT32 i = 0; i < numItems; i++)
  {
    UINT32 value = array[i];
    if (value <= subValue)
      value = kEmptyHashValue;
    else
      value -= subValue;
    array[i] = value;
  }
}

void CInTree::Normalize()
{
  UINT32 startItem = _pos - _historySize;
  UINT32 subValue = startItem - 1;
  // NormalizeLinks((CIndex *)(_son + startItem), _historySize * 2, subValue);
  NormalizeLinks((CIndex *)_son, _cyclicBufferSize * 2, subValue);
  
  NormalizeLinks(_hash, kHashSize, subValue);

  #ifdef HASH_ARRAY_2
  NormalizeLinks(_hash2, kHash2Size, subValue);
  #ifdef HASH_ARRAY_3
  NormalizeLinks(_hash3, kHash3Size, subValue);
  #endif
  #endif

  ReduceOffsets(subValue);
}
 
}
