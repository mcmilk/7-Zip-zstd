// PatMain.h

#include "../../../../Common/Defs.h"
#include "../../../../Common/Alloc.h"

namespace PAT_NAMESPACE {

STDMETHODIMP CPatricia::SetCallback(IMatchFinderCallback *callback)
{
  m_Callback = callback;
  return S_OK;
}

void CPatricia::BeforeMoveBlock()
{
  if (m_Callback)
    m_Callback->BeforeChangingBufferPos();
  CLZInWindow::BeforeMoveBlock();
}

void CPatricia::AfterMoveBlock()
{
  if (m_Callback)
    m_Callback->AfterChangingBufferPos();
  CLZInWindow::AfterMoveBlock();
}

const UInt32 kMatchStartValue2 = 2;
const UInt32 kDescendantEmptyValue2 = kMatchStartValue2 - 1;
const UInt32 kDescendantsNotInitilized2 = kDescendantEmptyValue2 - 1;

#ifdef __HASH_3

static const UInt32 kNumHashBytes = 3;
static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);

static const UInt32 kNumHash2Bytes = 2;
static const UInt32 kHash2Size = 1 << (8 * kNumHash2Bytes);
static const UInt32 kPrevHashSize = kNumHash2Bytes;

#else

static const UInt32 kNumHashBytes = 2;
static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
static const UInt32 kPrevHashSize = 0;

#endif


CPatricia::CPatricia():
  m_HashDescendants(0),
  #ifdef __HASH_3
  m_Hash2Descendants(0),
  #endif
  m_Nodes(0),
  m_TmpBacks(0)
{
}

CPatricia::~CPatricia()
{
  FreeMemory();
}

void CPatricia::FreeMemory()
{
  MyFree(m_TmpBacks);
  m_TmpBacks = 0;

  ::BigFree(m_Nodes);
  m_Nodes = 0;

  ::BigFree(m_HashDescendants);
  m_HashDescendants = 0;

  #ifdef __HASH_3

  ::BigFree(m_Hash2Descendants);
  m_Hash2Descendants = 0;

  CLZInWindow::Free();

  #endif
}
  
STDMETHODIMP CPatricia::Create(UInt32 historySize, UInt32 keepAddBufferBefore, 
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter)
{
  FreeMemory();
  int kNumBitsInNumSameBits = sizeof(CSameBitsType) * 8;
  if (kNumBitsInNumSameBits < 32 && ((matchMaxLen * MY_BYTE_SIZE) > ((UInt32)1 << kNumBitsInNumSameBits)))
    return E_INVALIDARG;

  const UInt32 kAlignMask = (1 << 16) - 1;
  UInt32 windowReservSize = historySize;
  windowReservSize += kAlignMask;
  windowReservSize &= ~(kAlignMask);

  const UInt32 kMinReservSize = (1 << 19);
  if (windowReservSize < kMinReservSize)
    windowReservSize = kMinReservSize;
  windowReservSize += 256;

  if (!CLZInWindow::Create(historySize + keepAddBufferBefore, 
      matchMaxLen + keepAddBufferAfter, windowReservSize))
    return E_OUTOFMEMORY;

  _sizeHistory = historySize;
  _matchMaxLen = matchMaxLen;
  m_HashDescendants = (CDescendant *)BigAlloc(kHashSize * sizeof(CDescendant));
  if (m_HashDescendants == 0)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }

  #ifdef __HASH_3
  m_Hash2Descendants = (CDescendant *)BigAlloc(kHash2Size  * sizeof(CDescendant));
  if (m_Hash2Descendants == 0)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
  #endif
  
  #ifdef __AUTO_REMOVE
  
  #ifdef __HASH_3
  m_NumNodes = historySize + _sizeHistory * 4 / 8 + (1 << 19);
  #else
  m_NumNodes = historySize + _sizeHistory * 4 / 8 + (1 << 10);
  #endif
  
  #else
  
  UInt32 m_NumNodes = historySize;
  
  #endif
  
  const UInt32 kMaxNumNodes = UInt32(1) << (sizeof(CIndex) * 8 - 1);
  if (m_NumNodes + 32 > kMaxNumNodes)
    return E_INVALIDARG;
  
  // m_Nodes = (CNode *)::BigAlloc((m_NumNodes + 2) * sizeof(CNode));
  m_Nodes = (CNode *)::BigAlloc((m_NumNodes + 12) * sizeof(CNode));
  if (m_Nodes == 0)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
  
  m_TmpBacks = (UInt32 *)MyAlloc((_matchMaxLen + 1) * sizeof(UInt32));
  if (m_TmpBacks == 0)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
  return S_OK;
}

STDMETHODIMP CPatricia::Init(ISequentialInStream *aStream)
{
  RINOK(CLZInWindow::Init(aStream));

  // memset(m_HashDescendants, 0xFF, kHashSize * sizeof(m_HashDescendants[0]));

  #ifdef __HASH_3
  for (UInt32 i = 0; i < kHash2Size; i++)
    m_Hash2Descendants[i].MatchPointer = kDescendantsNotInitilized2;
  #else
  for (UInt32 i = 0; i < kHashSize; i++)
    m_HashDescendants[i].MakeEmpty();
  #endif

  m_Nodes[0].NextFreeNode = 1;
  m_FreeNode = 0;
  m_FreeNodeMax = 0;
  #ifdef __AUTO_REMOVE
  m_NumUsedNodes = 0;
  #else
  m_SpecialRemoveMode = false;
  #endif
  m_SpecialMode = false;
  return S_OK;
}

STDMETHODIMP_(void) CPatricia::ReleaseStream()
{
  // CLZInWindow::ReleaseStream();
}

// pos = _pos + kNumHashBytes
// fullCurrentLimit = currentLimit + kNumHashBytes
// fullMatchLen = matchLen + kNumHashBytes

void CPatricia::ChangeLastMatch(UInt32 hashValue)
{
  UInt32 pos = _pos + kNumHashBytes - 1;
  UInt32 descendantIndex;
  const Byte *currentBytePointer = _buffer + pos;
  UInt32 numLoadedBits = 0;
  Byte curByte = 0;  // = 0 to disable warning of GCC
  CNodePointer node = &m_Nodes[m_HashDescendants[hashValue].NodePointer];

  while(true)
  {
    UInt32 numSameBits = node->NumSameBits;
    if(numSameBits > 0)
    {
      if (numLoadedBits < numSameBits)
      {
        numSameBits -= numLoadedBits;
        currentBytePointer += (numSameBits / MY_BYTE_SIZE);
        numSameBits %= MY_BYTE_SIZE;
        curByte = *currentBytePointer++;
        numLoadedBits = MY_BYTE_SIZE; 
      }
      curByte >>= numSameBits;
      numLoadedBits -= numSameBits;
    }
    if(numLoadedBits == 0)
    {
      curByte = *currentBytePointer++;
      numLoadedBits = MY_BYTE_SIZE; 
    }
    descendantIndex = (curByte & kSubNodesMask);
    node->LastMatch = pos;
    numLoadedBits -= kNumSubBits;
    curByte >>= kNumSubBits;
    if(node->Descendants[descendantIndex].IsNode())
      node = &m_Nodes[node->Descendants[descendantIndex].NodePointer];
    else
      break;
  }
  node->Descendants[descendantIndex].MatchPointer = pos + kMatchStartValue;
}

UInt32 CPatricia::GetLongestMatch(UInt32 *distances)
{
  UInt32 fullCurrentLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    fullCurrentLimit = _matchMaxLen;
  else
  {
    fullCurrentLimit = _streamPos - _pos;
    if(fullCurrentLimit < kNumHashBytes)
      return 0; 
  }
  UInt32 pos = _pos + kNumHashBytes;

  #ifdef __HASH_3
  UInt32 hash2Value = ((UInt32(_buffer[_pos])) << 8) | _buffer[_pos + 1];
  UInt32 hashValue = (hash2Value << 8) | _buffer[_pos + 2];
  CDescendant &hash2Descendant = m_Hash2Descendants[hash2Value];
  CDescendant &hashDescendant = m_HashDescendants[hashValue];
  if(hash2Descendant.MatchPointer <= kDescendantEmptyValue2)
  {
    if(hash2Descendant.MatchPointer == kDescendantsNotInitilized2)
    {
      UInt32 base = hashValue & 0xFFFF00;
      for (UInt32 i = 0; i < 0x100; i++)
        m_HashDescendants[base + i].MakeEmpty();
    }
    hash2Descendant.MatchPointer = pos + kMatchStartValue2;
    hashDescendant.MatchPointer = pos + kMatchStartValue;
    return 0;
  }

  distances[kNumHash2Bytes] = pos - (hash2Descendant.MatchPointer - kMatchStartValue2) - 1;
  hash2Descendant.MatchPointer = pos + kMatchStartValue2;
  #ifdef __AUTO_REMOVE
  if (distances[kNumHash2Bytes] >= _sizeHistory)
  {
    if (hashDescendant.IsNode())
      RemoveNode(hashDescendant.NodePointer);
    hashDescendant.MatchPointer = pos + kMatchStartValue;
    return 0;
  }
  #endif
  if (fullCurrentLimit == kNumHash2Bytes)
    return kNumHash2Bytes;

  #else
  UInt32 hashValue = UInt32(GetIndexByte(1))  | (UInt32(GetIndexByte(0)) << 8);
  CDescendant &hashDescendant = m_HashDescendants[hashValue];
  #endif


  if(m_SpecialMode)
  {
    if(hashDescendant.IsMatch())
      m_NumNotChangedCycles = 0;
    if(m_NumNotChangedCycles >= _sizeHistory - 1)
    {
      ChangeLastMatch(hashValue);
      m_NumNotChangedCycles = 0;
    }
    if(GetIndexByte(fullCurrentLimit - 1) == GetIndexByte(fullCurrentLimit - 2)) 
    {
      if(hashDescendant.IsMatch())
        hashDescendant.MatchPointer = pos + kMatchStartValue;
      else
        m_NumNotChangedCycles++;
      for(UInt32 i = kNumHashBytes; i <= fullCurrentLimit; i++)
        distances[i] = 0;
      return fullCurrentLimit;
    }
    else if(m_NumNotChangedCycles > 0)
      ChangeLastMatch(hashValue);
    m_SpecialMode = false;
  }

  if(hashDescendant.IsEmpty())
  {
    hashDescendant.MatchPointer = pos + kMatchStartValue;
    return kPrevHashSize;
  }

  UInt32 currentLimit = fullCurrentLimit - kNumHashBytes;

  if(hashDescendant.IsMatch())
  {
    CMatchPointer matchPointer = hashDescendant.MatchPointer;
    UInt32 backReal = pos - (matchPointer - kMatchStartValue);
    UInt32 back = backReal - 1;
    #ifdef __AUTO_REMOVE
    if (back >= _sizeHistory)
    {
      hashDescendant.MatchPointer = pos + kMatchStartValue;
      return kPrevHashSize;
    }
    #endif

    UInt32 matchLen;
    distances += kNumHashBytes;
    Byte *buffer = _buffer + pos;
    for(matchLen = 0; true; matchLen++)
    {
      *distances++ = back;
      if (matchLen == currentLimit)
      {
        hashDescendant.MatchPointer = pos + kMatchStartValue;
        return kNumHashBytes + matchLen;
      }
      if (buffer[matchLen] != buffer[(size_t)matchLen - backReal])
        break;
    }
     
    // UInt32 matchLen = GetMatchLen(kNumHashBytes, back, currentLimit);
    
    UInt32 fullMatchLen = matchLen + kNumHashBytes; 
    hashDescendant.NodePointer = m_FreeNode;
    CNodePointer node = &m_Nodes[m_FreeNode];
    m_FreeNode = node->NextFreeNode;
    #ifdef __AUTO_REMOVE
    m_NumUsedNodes++;
    #endif
    if (m_FreeNode > m_FreeNodeMax)
    {
      m_FreeNodeMax = m_FreeNode;
      m_Nodes[m_FreeNode].NextFreeNode = m_FreeNode + 1;
    }
      
    for (UInt32 i = 0; i < kNumSubNodes; i++)
      node->Descendants[i].NodePointer = kDescendantEmptyValue;
    node->LastMatch = pos;
      
    Byte byteNew = GetIndexByte(fullMatchLen);
    Byte byteOld = GetIndexByte(fullMatchLen - backReal);
    Byte bitsNew, bitsOld;
    UInt32 numSameBits = matchLen * MY_BYTE_SIZE;
    while (true)
    {
      bitsNew = (byteNew & kSubNodesMask);
      bitsOld = (byteOld & kSubNodesMask);
      if(bitsNew != bitsOld) 
        break;
      byteNew >>= kNumSubBits;
      byteOld >>= kNumSubBits;
      numSameBits += kNumSubBits;
    }
    node->NumSameBits = CSameBitsType(numSameBits);
    node->Descendants[bitsNew].MatchPointer = pos + kMatchStartValue;
    node->Descendants[bitsOld].MatchPointer = matchPointer;
    return fullMatchLen;
  }
  const Byte *baseCurrentBytePointer = _buffer + pos;
  const Byte *currentBytePointer = baseCurrentBytePointer;
  UInt32 numLoadedBits = 0;
  Byte curByte = 0;
  CIndex *nodePointerPointer = &hashDescendant.NodePointer;
  CNodePointer node = &m_Nodes[*nodePointerPointer];
  distances += kNumHashBytes;
  const Byte *bytePointerLimit = baseCurrentBytePointer + currentLimit;
  const Byte *currentAddingOffset = _buffer;

  #ifdef __AUTO_REMOVE
  UInt32 lowPos;
  if (pos > _sizeHistory)
    lowPos = pos - _sizeHistory;
  else
    lowPos = 0;
  #endif

  while(true)
  {
    #ifdef __AUTO_REMOVE
    if (node->LastMatch < lowPos)
    {
      RemoveNode(*nodePointerPointer);
      *nodePointerPointer = pos + kMatchStartValue;
      if (currentBytePointer == baseCurrentBytePointer)
        return kPrevHashSize;
      return kNumHashBytes + (currentBytePointer - baseCurrentBytePointer - 1);
    }
    #endif
    if(numLoadedBits == 0)
    {
      *distances++ = pos - node->LastMatch - 1;
      if(currentBytePointer >= bytePointerLimit)
      {
        for (UInt32 i = 0; i < kNumSubNodes; i++)
          node->Descendants[i].MatchPointer = pos + kMatchStartValue;
        node->LastMatch = pos;
        node->NumSameBits = 0;
        return fullCurrentLimit;
      }
      curByte = (*currentBytePointer++);
      currentAddingOffset++;
      numLoadedBits = MY_BYTE_SIZE; 
    }
    UInt32 numSameBits = node->NumSameBits;
    if(numSameBits > 0)
    {
      Byte byteXOR = ((*(currentAddingOffset + node->LastMatch -1)) >> 
          (MY_BYTE_SIZE - numLoadedBits)) ^ curByte;
      while(numLoadedBits <= numSameBits)
      {
        if(byteXOR != 0)
        {
          AddInternalNode(node, nodePointerPointer, curByte, byteXOR,
              numSameBits, pos);
          return kNumHashBytes + (currentBytePointer - baseCurrentBytePointer - 1);
        }
        *distances++ = pos - node->LastMatch - 1;
        numSameBits -= numLoadedBits;
        if(currentBytePointer >= bytePointerLimit)
        {
          for (UInt32 i = 0; i < kNumSubNodes; i++)
            node->Descendants[i].MatchPointer = pos + kMatchStartValue;
          node->LastMatch = pos;
          node->NumSameBits = CSameBitsType(node->NumSameBits - numSameBits);
          return fullCurrentLimit;
        }
        numLoadedBits = MY_BYTE_SIZE; 
        curByte = (*currentBytePointer++);
        byteXOR = curByte ^ (*(currentAddingOffset + node->LastMatch));
        currentAddingOffset++;
      }
      if((byteXOR & ((1 << numSameBits) - 1)) != 0)
      {
        AddInternalNode(node, nodePointerPointer, curByte, byteXOR,
            numSameBits, pos);
        return kNumHashBytes + (currentBytePointer - baseCurrentBytePointer - 1);
      }
      curByte >>= numSameBits;
      numLoadedBits -= numSameBits;
    }
    UInt32 descendantIndex = (curByte & kSubNodesMask);
    numLoadedBits -= kNumSubBits;
    nodePointerPointer = &node->Descendants[descendantIndex].NodePointer;
    UInt32 nextNodeIndex = *nodePointerPointer;
    node->LastMatch = pos;
    if (nextNodeIndex < kDescendantEmptyValue)
    {
      curByte >>= kNumSubBits;
      node = &m_Nodes[nextNodeIndex];
    }
    else if (nextNodeIndex == kDescendantEmptyValue)
    {
      node->Descendants[descendantIndex].MatchPointer = pos + kMatchStartValue;
      return kNumHashBytes + (currentBytePointer - baseCurrentBytePointer - 1);
    }
    else 
      break;
  }
 
  UInt32 descendantIndex = (curByte & kSubNodesMask);
  curByte >>= kNumSubBits;
  CMatchPointer matchPointer = node->Descendants[descendantIndex].MatchPointer;
  CMatchPointer realMatchPointer;
  realMatchPointer = matchPointer - kMatchStartValue;

  #ifdef __AUTO_REMOVE
  if (realMatchPointer < lowPos)
  {
    node->Descendants[descendantIndex].MatchPointer = pos + kMatchStartValue;
    return kNumHashBytes + (currentBytePointer - baseCurrentBytePointer - 1);
  }
  #endif

  Byte byteXOR;
  UInt32 numSameBits = 0;
  if(numLoadedBits != 0)
  {
    Byte matchByte = *(currentAddingOffset + realMatchPointer -1);  
    matchByte >>= (MY_BYTE_SIZE - numLoadedBits);
    byteXOR = matchByte ^ curByte;
    if(byteXOR != 0)
    {
      AddLeafNode(node, curByte, byteXOR, numSameBits, pos, descendantIndex);
      return kNumHashBytes + (currentBytePointer - baseCurrentBytePointer - 1);
    }
    numSameBits += numLoadedBits;
  }

  const Byte *matchBytePointer = _buffer + realMatchPointer + 
      (currentBytePointer - baseCurrentBytePointer);
  for(; currentBytePointer < bytePointerLimit; numSameBits += MY_BYTE_SIZE)
  {
    curByte = (*currentBytePointer++);
    *distances++ = pos - realMatchPointer - 1;
    byteXOR = curByte ^ (*matchBytePointer++);
    if(byteXOR != 0)
    {
      AddLeafNode(node, curByte, byteXOR, numSameBits, pos, descendantIndex);
      return kNumHashBytes + (currentBytePointer - baseCurrentBytePointer - 1);
    }
  }
  *distances = pos - realMatchPointer - 1;
  node->Descendants[descendantIndex].MatchPointer = pos + kMatchStartValue;

  if(*distances == 0)
  {
    m_SpecialMode = true;
    m_NumNotChangedCycles = 0;
  }
  return fullCurrentLimit;
}

STDMETHODIMP_(void) CPatricia::DummyLongestMatch()
{
  GetLongestMatch(m_TmpBacks);
}


// ------------------------------------
// Remove Match

typedef Byte CRemoveDataWord;

static const int kSizeRemoveDataWordInBits = MY_BYTE_SIZE * sizeof(CRemoveDataWord);

#ifndef __AUTO_REMOVE

void CPatricia::RemoveMatch()
{
  if(m_SpecialRemoveMode)
  {
    if(GetIndexByte(_matchMaxLen - 1 - _sizeHistory) ==
        GetIndexByte(_matchMaxLen - _sizeHistory))
      return;
    m_SpecialRemoveMode = false;
  }
  UInt32 pos = _pos + kNumHashBytes - _sizeHistory;

  #ifdef __HASH_3
  const Byte *pp = _buffer + _pos - _sizeHistory;
  UInt32 hash2Value = ((UInt32(pp[0])) << 8) | pp[1];
  UInt32 hashValue = (hash2Value << 8) | pp[2];
  CDescendant &hashDescendant = m_HashDescendants[hashValue];
  CDescendant &hash2Descendant = m_Hash2Descendants[hash2Value];
  if (hash2Descendant >= kMatchStartValue2)
    if(hash2Descendant.MatchPointer == pos + kMatchStartValue2)
      hash2Descendant.MatchPointer = kDescendantEmptyValue2;
  #else
  UInt32 hashValue = UInt32(GetIndexByte(1 - _sizeHistory))  | 
      (UInt32(GetIndexByte(0 - _sizeHistory)) << 8);
  CDescendant &hashDescendant = m_HashDescendants[hashValue];
  #endif
    
  if(hashDescendant.IsEmpty())
    return;
  if(hashDescendant.IsMatch())
  {
    if(hashDescendant.MatchPointer == pos + kMatchStartValue)
      hashDescendant.MakeEmpty();
    return;
  }
  
  UInt32 descendantIndex;
  const CRemoveDataWord *currentPointer = (const CRemoveDataWord *)(_buffer + pos);
  UInt32 numLoadedBits = 0;
  CRemoveDataWord curWord = 0; // = 0 to disable GCC warning

  CIndex *nodePointerPointer = &hashDescendant.NodePointer;

  CNodePointer node = &m_Nodes[hashDescendant.NodePointer];
  
  while(true)
  {
    if(numLoadedBits == 0)
    {
      curWord = *currentPointer++;
      numLoadedBits = kSizeRemoveDataWordInBits; 
    }
    UInt32 numSameBits = node->NumSameBits;
    if(numSameBits > 0)
    {
      if (numLoadedBits <= numSameBits)
      {
        numSameBits -= numLoadedBits;
        currentPointer += (numSameBits / kSizeRemoveDataWordInBits);
        numSameBits %= kSizeRemoveDataWordInBits;
        curWord = *currentPointer++;
        numLoadedBits = kSizeRemoveDataWordInBits; 
      }
      curWord >>= numSameBits;
      numLoadedBits -= numSameBits;
    }
    descendantIndex = (curWord & kSubNodesMask);
    numLoadedBits -= kNumSubBits;
    curWord >>= kNumSubBits;
    UInt32 nextNodeIndex = node->Descendants[descendantIndex].NodePointer;
    if (nextNodeIndex < kDescendantEmptyValue)
    {
      nodePointerPointer = &node->Descendants[descendantIndex].NodePointer;
      node = &m_Nodes[nextNodeIndex];
    }
    else
      break;
  }
  if (node->Descendants[descendantIndex].MatchPointer != pos + kMatchStartValue)
  {
    const Byte *currentBytePointer = _buffer + _pos - _sizeHistory;
    const Byte *currentBytePointerLimit = currentBytePointer + _matchMaxLen;
    for(;currentBytePointer < currentBytePointerLimit; currentBytePointer++)
      if(*currentBytePointer != *(currentBytePointer+1))
        return;
    m_SpecialRemoveMode = true;
    return;
  }

  UInt32 numNodes = 0, numMatches = 0;

  UInt32 i;
  for (i = 0; i < kNumSubNodes; i++)
  {
    UInt32 nodeIndex = node->Descendants[i].NodePointer;
    if (nodeIndex < kDescendantEmptyValue)
      numNodes++;
    else if (nodeIndex > kDescendantEmptyValue)
      numMatches++;
  }
  numMatches -= 1;
  if (numNodes + numMatches > 1)
  {
    node->Descendants[descendantIndex].MakeEmpty();
    return;
  }
  if(numNodes == 1)
  {
    UInt32 i;
    for (i = 0; i < kNumSubNodes; i++)
      if (node->Descendants[i].IsNode())
        break;
    UInt32 nextNodeIndex = node->Descendants[i].NodePointer;
    CNodePointer nextNode = &m_Nodes[nextNodeIndex];
    nextNode->NumSameBits += node->NumSameBits + kNumSubBits;
    *node = *nextNode;

    nextNode->NextFreeNode = m_FreeNode;
    m_FreeNode = nextNodeIndex;
    return;
  }
  UInt32 matchPointer = 0; // = 0 to disable GCC warning
  for (i = 0; i < kNumSubNodes; i++)
    if (node->Descendants[i].IsMatch() && i != descendantIndex)
    {
      matchPointer = node->Descendants[i].MatchPointer;
      break;
    }
  node->NextFreeNode = m_FreeNode;
  m_FreeNode = *nodePointerPointer;
  *nodePointerPointer = matchPointer;
}
#endif


// Commented code is more correct, but it gives warning 
// on GCC: (1 << 32)
// So we use kMatchStartValue twice:
// kMatchStartValue = UInt32(1) << (kNumBitsInIndex - 1);
// must be defined in Pat.h
/*
const UInt32 kNormalizeStartPos = (UInt32(1) << (kNumBitsInIndex)) - 
    kMatchStartValue - kNumHashBytes - 1;
*/
const UInt32 kNormalizeStartPos = kMatchStartValue - kNumHashBytes - 1;

STDMETHODIMP CPatricia::MovePos()
{
  #ifndef __AUTO_REMOVE
  if(_pos >= _sizeHistory)
    RemoveMatch();
  #endif
  RINOK(CLZInWindow::MovePos());
  #ifdef __AUTO_REMOVE
  if (m_NumUsedNodes >= m_NumNodes)
    TestRemoveNodes();
  #endif
  if (_pos >= kNormalizeStartPos)
  {
    #ifdef __AUTO_REMOVE
    TestRemoveNodesAndNormalize();
    #else
    Normalize();
    #endif
  }
  return S_OK;
}

#ifndef __AUTO_REMOVE

void CPatricia::NormalizeDescendant(CDescendant &descendant, UInt32 subValue)
{
  if (descendant.IsEmpty())
    return;
  if (descendant.IsMatch())
    descendant.MatchPointer = descendant.MatchPointer - subValue;
  else
  {
    CNode &node = m_Nodes[descendant.NodePointer];
    node.LastMatch = node.LastMatch - subValue;
    for (UInt32 i = 0; i < kNumSubNodes; i++)
       NormalizeDescendant(node.Descendants[i], subValue);
  }
}

void CPatricia::Normalize()
{
  UInt32 subValue = _pos - _sizeHistory;
  CLZInWindow::ReduceOffsets(subValue);
  
  #ifdef __HASH_3

  for(UInt32 hash = 0; hash < kHash2Size; hash++)
  {
    CDescendant &descendant = m_Hash2Descendants[hash];
    if (descendant.MatchPointer != kDescendantsNotInitilized2)
    {
      UInt32 base = hash << 8;
      for (UInt32 i = 0; i < 0x100; i++)
        NormalizeDescendant(m_HashDescendants[base + i], subValue);
    }
    if (descendant.MatchPointer < kMatchStartValue2)
      continue;
    descendant.MatchPointer = descendant.MatchPointer - subValue;
  }
  
  #else
  
  for(UInt32 hash = 0; hash < kHashSize; hash++)
    NormalizeDescendant(m_HashDescendants[hash], subValue);
  
  #endif

}

#else

void CPatricia::TestRemoveDescendant(CDescendant &descendant, UInt32 limitPos)
{
  CNode &node = m_Nodes[descendant.NodePointer];
  UInt32 numChilds = 0;
  UInt32 childIndex = 0; // = 0 to disable GCC warning
  for (UInt32 i = 0; i < kNumSubNodes; i++)
  {
    CDescendant &descendant2 = node.Descendants[i];
    if (descendant2.IsEmpty())
      continue;
    if (descendant2.IsMatch())
    {
      if (descendant2.MatchPointer < limitPos)
        descendant2.MakeEmpty();
      else
      {
        numChilds++;
        childIndex = i;
      }
    }
    else
    {
      TestRemoveDescendant(descendant2, limitPos);
      if (!descendant2.IsEmpty())
      {
        numChilds++;
        childIndex = i;
      }
    }
  }
  if (numChilds > 1)
    return;

  CIndex nodePointerTemp = descendant.NodePointer;
  if (numChilds == 1)
  {
    const CDescendant &descendant2 = node.Descendants[childIndex];
    if (descendant2.IsNode())
      m_Nodes[descendant2.NodePointer].NumSameBits += node.NumSameBits + kNumSubBits;
    descendant = descendant2;
  }
  else
    descendant.MakeEmpty();
  node.NextFreeNode = m_FreeNode;
  m_FreeNode = nodePointerTemp;
  m_NumUsedNodes--;
}

void CPatricia::RemoveNode(UInt32 index)
{
  CNode &node = m_Nodes[index];
  for (UInt32 i = 0; i < kNumSubNodes; i++)
  {
    CDescendant &descendant2 = node.Descendants[i];
    if (descendant2.IsNode())
      RemoveNode(descendant2.NodePointer);
  }
  node.NextFreeNode = m_FreeNode;
  m_FreeNode = index;
  m_NumUsedNodes--;
}

void CPatricia::TestRemoveNodes()
{
  UInt32 limitPos = kMatchStartValue + _pos - _sizeHistory + kNumHashBytes;
  
  #ifdef __HASH_3
  
  UInt32 limitPos2 = kMatchStartValue2 + _pos - _sizeHistory + kNumHashBytes;
  for(UInt32 hash = 0; hash < kHash2Size; hash++)
  {
    CDescendant &descendant = m_Hash2Descendants[hash];
    if (descendant.MatchPointer != kDescendantsNotInitilized2)
    {
      UInt32 base = hash << 8;
      for (UInt32 i = 0; i < 0x100; i++)
      {
        CDescendant &descendant = m_HashDescendants[base + i];
        if (descendant.IsEmpty())
          continue;
        if (descendant.IsMatch())
        {
          if (descendant.MatchPointer < limitPos)
            descendant.MakeEmpty();
        }
        else
          TestRemoveDescendant(descendant, limitPos);
      }
    }
    if (descendant.MatchPointer < kMatchStartValue2)
      continue;
    if (descendant.MatchPointer < limitPos2)
      descendant.MatchPointer = kDescendantEmptyValue2;
  }
  
  #else
  
  for(UInt32 hash = 0; hash < kHashSize; hash++)
  {
    CDescendant &descendant = m_HashDescendants[hash];
    if (descendant.IsEmpty())
      continue;
    if (descendant.IsMatch())
    {
      if (descendant.MatchPointer < limitPos)
        descendant.MakeEmpty();
    }
    else
      TestRemoveDescendant(descendant, limitPos);
  }
  
  #endif
}

void CPatricia::TestRemoveAndNormalizeDescendant(CDescendant &descendant, 
    UInt32 limitPos, UInt32 subValue)
{
  if (descendant.IsEmpty())
    return;
  if (descendant.IsMatch())
  {
    if (descendant.MatchPointer < limitPos)
      descendant.MakeEmpty();
    else
      descendant.MatchPointer = descendant.MatchPointer - subValue;
    return;
  }
  CNode &node = m_Nodes[descendant.NodePointer];
  UInt32 numChilds = 0;
  UInt32 childIndex = 0; // = 0 to disable GCC warning
  for (UInt32 i = 0; i < kNumSubNodes; i++)
  {
    CDescendant &descendant2 = node.Descendants[i];
    TestRemoveAndNormalizeDescendant(descendant2, limitPos, subValue);
    if (!descendant2.IsEmpty())
    {
      numChilds++;
      childIndex = i;
    }
  }
  if (numChilds > 1)
  {
    node.LastMatch = node.LastMatch - subValue;
    return;
  }

  CIndex nodePointerTemp = descendant.NodePointer;
  if (numChilds == 1)
  {
    const CDescendant &descendant2 = node.Descendants[childIndex];
    if (descendant2.IsNode())
      m_Nodes[descendant2.NodePointer].NumSameBits += node.NumSameBits + kNumSubBits;
    descendant = descendant2;
  }
  else
    descendant.MakeEmpty();
  node.NextFreeNode = m_FreeNode;
  m_FreeNode = nodePointerTemp;
  m_NumUsedNodes--;
}

void CPatricia::TestRemoveNodesAndNormalize()
{
  UInt32 subValue = _pos - _sizeHistory;
  UInt32 limitPos = kMatchStartValue + _pos - _sizeHistory + kNumHashBytes;
  CLZInWindow::ReduceOffsets(subValue);

  #ifdef __HASH_3
  
  UInt32 limitPos2 = kMatchStartValue2 + _pos - _sizeHistory + kNumHashBytes;
  for(UInt32 hash = 0; hash < kHash2Size; hash++)
  {
    CDescendant &descendant = m_Hash2Descendants[hash];
    if (descendant.MatchPointer != kDescendantsNotInitilized2)
    {
      UInt32 base = hash << 8;
      for (UInt32 i = 0; i < 0x100; i++)
        TestRemoveAndNormalizeDescendant(m_HashDescendants[base + i], limitPos, subValue);
    }
    if (descendant.MatchPointer < kMatchStartValue2)
      continue;
    if (descendant.MatchPointer < limitPos2)
      descendant.MatchPointer = kDescendantEmptyValue2;
    else
      descendant.MatchPointer = descendant.MatchPointer - subValue;
  }
  
  #else

  for(UInt32 hash = 0; hash < kHashSize; hash++)
    TestRemoveAndNormalizeDescendant(m_HashDescendants[hash], limitPos, subValue);

  #endif
}

#endif

STDMETHODIMP_(Byte) CPatricia::GetIndexByte(Int32 index)
{
  return CLZInWindow::GetIndexByte(index);
}

STDMETHODIMP_(UInt32) CPatricia::GetMatchLen(Int32 index, UInt32 back, UInt32 limit)
{
  return CLZInWindow::GetMatchLen(index, back, limit);
}

STDMETHODIMP_(UInt32) CPatricia::GetNumAvailableBytes()
{
  return CLZInWindow::GetNumAvailableBytes();
}

STDMETHODIMP_(const Byte *) CPatricia::GetPointerToCurrentPos()
{
  return CLZInWindow::GetPointerToCurrentPos();
}

}
