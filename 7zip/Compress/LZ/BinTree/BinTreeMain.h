// BinTreeMain.h

#include "../../../../Common/Defs.h"
#include "../../../../Common/CRC.h"
#include "../../../../Common/Alloc.h"

namespace BT_NAMESPACE {

#ifdef HASH_ARRAY_2
  static const UInt32 kHash2Size = 1 << 10;
  #ifdef HASH_ARRAY_3
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kNumHashBytes = 4;
    static const UInt32 kHash3Size = 1 << 18;
    #ifdef HASH_BIG
    static const UInt32 kHashSize = 1 << 23;
    #else
    static const UInt32 kHashSize = 1 << 20;
    #endif
  #else
    static const UInt32 kNumHashDirectBytes = 3;
    static const UInt32 kNumHashBytes = 3;
    static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#else
  #ifdef HASH_ZIP 
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kNumHashBytes = 3;
    static const UInt32 kHashSize = 1 << 16;
  #else
    static const UInt32 kNumHashDirectBytes = 2;
    static const UInt32 kNumHashBytes = 2;
    static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#endif


CInTree::CInTree():
  _hash(0),
  #ifdef HASH_ARRAY_2
  _hash2(0),
  #ifdef HASH_ARRAY_3
  _hash3(0),
  #endif
  #endif
  _son(0),
  _cutValue(0xFF)
{
}

void CInTree::FreeMemory()
{
  BigFree(_son);
  _son = 0;
  BigFree(_hash);
  _hash = 0;
  CLZInWindow::Free();
}

CInTree::~CInTree()
{ 
  FreeMemory();
}

HRESULT CInTree::Create(UInt32 sizeHistory, UInt32 keepAddBufferBefore, 
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter, UInt32 sizeReserv)
{
  FreeMemory();
  if (!CLZInWindow::Create(sizeHistory + keepAddBufferBefore, 
      matchMaxLen + keepAddBufferAfter, sizeReserv))
    return E_OUTOFMEMORY;
  
  if (_blockSize + 256 > kMaxValForNormalize)
    return E_INVALIDARG;
  
  _historySize = sizeHistory;
  _matchMaxLen = matchMaxLen;
  
  _cyclicBufferSize = sizeHistory + 1;
  
  
  UInt32 size = kHashSize;
  #ifdef HASH_ARRAY_2
  size += kHash2Size;
  #ifdef HASH_ARRAY_3
  size += kHash3Size;
  #endif
  #endif
  
  _son = (CPair *)BigAlloc((_cyclicBufferSize + 1) * sizeof(CPair));
  if (_son == 0)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
  
  // UInt32 numBundles = (_cyclicBufferSize + kNumPairsInBundle) >> kNumBundleBits;
  // _son = (CBundle *)::VirtualAlloc(0, numBundles * sizeof(CBundle), MEM_COMMIT, PAGE_READWRITE);
  _hash = (CIndex *)BigAlloc((size + 1) * sizeof(CIndex));
  if (_hash == 0)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
  
  // _hash = &m_RightBase[_blockSize];
  #ifdef HASH_ARRAY_2
  _hash2 = &_hash[kHashSize]; 
  #ifdef HASH_ARRAY_3
  _hash3 = &_hash2[kHash2Size]; 
  #endif
  #endif
  return S_OK;
}

static const UInt32 kEmptyHashValue = 0;

HRESULT CInTree::Init(ISequentialInStream *stream)
{
  RINOK(CLZInWindow::Init(stream));
  UInt32 i;
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

  ReduceOffsets(-1);
  return S_OK;
}


#ifdef HASH_ARRAY_2
#ifdef HASH_ARRAY_3
inline UInt32 Hash(const Byte *pointer, UInt32 &hash2Value, UInt32 &hash3Value)
{
  UInt32 temp = CCRC::Table[pointer[0]] ^ pointer[1];
  hash2Value = temp & (kHash2Size - 1);
  hash3Value = (temp ^ (UInt32(pointer[2]) << 8)) & (kHash3Size - 1);
  return (temp ^ (UInt32(pointer[2]) << 8) ^ (CCRC::Table[pointer[3]] << 5)) & 
      (kHashSize - 1);
}
#else // no HASH_ARRAY_3
inline UInt32 Hash(const Byte *pointer, UInt32 &hash2Value)
{
  hash2Value = (CCRC::Table[pointer[0]] ^ pointer[1]) & (kHash2Size - 1);
  return ((UInt32(pointer[0]) << 16)) | ((UInt32(pointer[1]) << 8)) | pointer[2];
}
#endif // HASH_ARRAY_3
#else // no HASH_ARRAY_2
#ifdef HASH_ZIP 
inline UInt32 Hash(const Byte *pointer)
{
  return ((UInt32(pointer[0]) << 8) ^ 
      CCRC::Table[pointer[1]] ^ pointer[2]) & (kHashSize - 1);
}
#else // no HASH_ZIP 
inline UInt32 Hash(const Byte *pointer)
{
  return pointer[0] ^ (UInt32(pointer[1]) << 8);
}
#endif // HASH_ZIP
#endif // HASH_ARRAY_2

UInt32 CInTree::GetLongestMatch(UInt32 *distances)
{
  UInt32 lenLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    lenLimit = _matchMaxLen;
  else
  {
    lenLimit = _streamPos - _pos;
    if(lenLimit < kNumHashBytes)
      return 0; 
  }

  UInt32 matchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  Byte *cur = _buffer + _pos;

  /*
  if ((_cyclicBufferPos & kBundleMask) == 0)
  {
    Byte *bytes = _son[_cyclicBufferPos >> kNumBundleBits].Bytes;
    UInt32 bundleLimit = kNumBundleBytes;
    if (bundleLimit > lenLimit)
      bundleLimit = lenLimit;
    for (UInt32 i = 0; i < bundleLimit; i++)
      bytes[i] = cur[i];
  }
  */
  
  UInt32 matchHashLenMax = 0;

  #ifdef HASH_ARRAY_2
  UInt32 hash2Value;
  #ifdef HASH_ARRAY_3
  UInt32 hash3Value;
  UInt32 hashValue = Hash(cur, hash2Value, hash3Value);
  #else
  UInt32 hashValue = Hash(cur, hash2Value);
  #endif
  #else
  UInt32 hashValue = Hash(cur);
  #endif

  UInt32 curMatch = _hash[hashValue];
  #ifdef HASH_ARRAY_2
  UInt32 curMatch2 = _hash2[hash2Value];
  #ifdef HASH_ARRAY_3
  UInt32 curMatch3 = _hash3[hash3Value];
  #endif
  _hash2[hash2Value] = _pos;
  bool matchLen2Exist = false;
  UInt32 len2Distance = 0;
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
  UInt32 matchLen3Exist = false;
  UInt32 len3Distance = 0;
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

  // UInt32 bi = _cyclicBufferPos >> kNumBundleBits;
  // UInt32 bo = _cyclicBufferPos & kBundleMask;
  // CPair &pair = _son[bi].Pairs[bo];
  CPair &pair = _son[_cyclicBufferPos];
  if(curMatch < matchMinPos)
  {
    pair.Left = kEmptyHashValue; 
    pair.Right = kEmptyHashValue; 

    #ifdef HASH_ARRAY_2
    distances[2] = len2Distance;
    #ifdef HASH_ARRAY_3
    distances[3] = len3Distance;
    #endif
    #endif

    return matchHashLenMax;
  }
  CIndex *ptrLeft = &pair.Right;
  CIndex *ptrRight = &pair.Left;

  UInt32 maxLen, minLeft, minRight;
  maxLen = minLeft = minRight = kNumHashDirectBytes;

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
  
  for(UInt32 count = _cutValue; count > 0; count--)
  {
    /*
    UInt32 delta = _pos - curMatch;
    UInt32 cyclicPos = (delta <= _cyclicBufferPos) ?
        (_cyclicBufferPos - delta):
        (_cyclicBufferPos - delta + _cyclicBufferSize);

    CBundle &bundle = _son[cyclicPos >> kNumBundleBits];
    UInt32 bo = cyclicPos & kBundleMask;
    CPair &pair = bundle.Pairs[bo];

    Byte *pby1 = bundle.Bytes + bo;
    UInt32 bundleLimit = kNumBundleBytes - bo;
    UInt32 currentLen = minSame;
    if (bundleLimit > lenLimit)
      bundleLimit = lenLimit;
    for(; currentLen < bundleLimit; currentLen++)
      if (pby1[currentLen] != cur[currentLen])
        break;
    if (currentLen >= bundleLimit)
    {
      pby1 = _buffer + curMatch;
      for(; currentLen < lenLimit; currentLen++)
        if (pby1[currentLen] != cur[currentLen])
          break;
    }
    */
    Byte *pby1 = _buffer + curMatch;
    // CIndex left = pair.Left; // it's prefetch
    UInt32 currentLen = MyMin(minLeft, minRight);
    for(; currentLen < lenLimit; currentLen++)
      if (pby1[currentLen] != cur[currentLen])
        break;
    UInt32 delta = _pos - curMatch;
    while (currentLen > maxLen)
      distances[++maxLen] = delta - 1;

    UInt32 cyclicPos = (delta <= _cyclicBufferPos) ?
        (_cyclicBufferPos - delta):
        (_cyclicBufferPos - delta + _cyclicBufferSize);
    CPair &pair = _son[cyclicPos];
    
    if (currentLen != lenLimit)
    {
      if (pby1[currentLen] < cur[currentLen])
      {
        *ptrRight = curMatch;
        ptrRight = &pair.Right;
        curMatch = pair.Right;
        if(currentLen > minLeft)
          minLeft = currentLen;
      }
      else
      {
        *ptrLeft = curMatch;
        ptrLeft = &pair.Left;
        curMatch = pair.Left;
        if(currentLen > minRight)
          minRight = currentLen;
      }
    }
    else
    {
      if(currentLen < _matchMaxLen)
      {
        *ptrLeft = curMatch;
        ptrLeft = &pair.Left;
        curMatch = pair.Left;
        if(currentLen > minRight)
          minRight = currentLen;
      }
      else
      {
        *ptrLeft = pair.Right;
        *ptrRight = pair.Left;

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
  UInt32 lenLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    lenLimit = _matchMaxLen;
  else
  {
    lenLimit = _streamPos - _pos;
    if(lenLimit < kNumHashBytes)
      return; 
  }
  UInt32 matchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  Byte *cur = _buffer + _pos;

  #ifdef HASH_ARRAY_2
  UInt32 hash2Value;
  #ifdef HASH_ARRAY_3
  UInt32 hash3Value;
  UInt32 hashValue = Hash(cur, hash2Value, hash3Value);
  _hash3[hash3Value] = _pos;
  #else
  UInt32 hashValue = Hash(cur, hash2Value);
  #endif
  _hash2[hash2Value] = _pos;
  #else
  UInt32 hashValue = Hash(cur);
  #endif

  UInt32 curMatch = _hash[hashValue];
  _hash[hashValue] = _pos;

  CPair &pair = _son[_cyclicBufferPos];

  if(curMatch < matchMinPos)
  {
    pair.Left = kEmptyHashValue; 
    pair.Right = kEmptyHashValue; 
    return;
  }
  CIndex *ptrLeft = &pair.Right;
  CIndex *ptrRight = &pair.Left;

  UInt32 maxLen, minLeft, minRight;
  maxLen = minLeft = minRight = kNumHashDirectBytes;
  for(UInt32 count = _cutValue; count > 0; count--)
  {
    Byte *pby1 = _buffer + curMatch;
    UInt32 currentLen = MyMin(minLeft, minRight);
    for(; currentLen < lenLimit; currentLen++)
      if (pby1[currentLen] != cur[currentLen])
        break;

    UInt32 delta = _pos - curMatch;
    UInt32 cyclicPos = (delta <= _cyclicBufferPos) ?
        (_cyclicBufferPos - delta):
        (_cyclicBufferPos - delta + _cyclicBufferSize);
    CPair &pair = _son[cyclicPos];

    if (currentLen != lenLimit)
    {
      if (pby1[currentLen] < cur[currentLen])
      {
        *ptrRight = curMatch;
        ptrRight = &pair.Right;
        curMatch = pair.Right;
        if(currentLen > minLeft)
          minLeft = currentLen;
      }
      else 
      {
        *ptrLeft = curMatch;
        ptrLeft = &pair.Left;
        curMatch = pair.Left;
        if(currentLen > minRight)
          minRight = currentLen;
      }
    }
    else
    {
      if(currentLen < _matchMaxLen)
      {
        *ptrLeft = curMatch;
        ptrLeft = &pair.Left;
        curMatch = pair.Left;
        if(currentLen > minRight)
          minRight = currentLen;
      }
      else
      {
        *ptrLeft = pair.Right;
        *ptrRight = pair.Left;
        return;
      }
    }
    if(curMatch < matchMinPos)
      break;
  }
  *ptrLeft = kEmptyHashValue;
  *ptrRight = kEmptyHashValue;
}

void CInTree::NormalizeLinks(CIndex *items, UInt32 numItems, UInt32 subValue)
{
  for (UInt32 i = 0; i < numItems; i++)
  {
    UInt32 value = items[i];
    if (value <= subValue)
      value = kEmptyHashValue;
    else
      value -= subValue;
    items[i] = value;
  }
}

void CInTree::Normalize()
{
  UInt32 startItem = _pos - _historySize;
  UInt32 subValue = startItem - 1;
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
