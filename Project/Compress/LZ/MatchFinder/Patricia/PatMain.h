// PatMain.h

#include "Common/Defs.h"

#include "Windows/Defs.h"

namespace PAT_NAMESPACE {

const UINT32 kMatchStartValue2 = 2;
const UINT32 kDescendantEmptyValue2 = kMatchStartValue2 - 1;
const UINT32 kDescendantsNotInitilized2 = kDescendantEmptyValue2 - 1;

#ifdef __HASH_3

static const UINT32 kNumHashBytes = 3;
static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);

static const UINT32 kNumHash2Bytes = 2;
static const UINT32 kHash2Size = 1 << (8 * kNumHash2Bytes);
static const UINT32 kPrevHashSize = kNumHash2Bytes;

#else

static const UINT32 kNumHashBytes = 2;
static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);
static const UINT32 kPrevHashSize = 0;

#endif


CPatricia::CPatricia():
  m_HashDescendants(0),
  #ifdef __HASH_3
  m_Hash2Descendants(0),
  #endif
  m_TmpBacks(0)
  {}
  
STDMETHODIMP CPatricia::Create(UINT32 aSizeHistory, UINT32 aKeepAddBufferBefore, 
    UINT32 aMatchMaxLen, UINT32 aKeepAddBufferAfter)
{
  const kNumBitsInNumSameBits = sizeof(CSameBitsType) * 8;
  if (kNumBitsInNumSameBits < 32 && ((aMatchMaxLen * MY_BYTE_SIZE) > (1 << kNumBitsInNumSameBits)))
    return E_INVALIDARG;

  const UINT32 kAlignMask = (1 << 16) - 1;
  UINT32 aWindowReservSize = aSizeHistory;
  aWindowReservSize += kAlignMask;
  aWindowReservSize &= ~(kAlignMask);

  const kMinReservSize = (1 << 19);
  if (aWindowReservSize < kMinReservSize)
    aWindowReservSize = kMinReservSize;
  aWindowReservSize += 256;

  try 
  {
    CIn::Create(aSizeHistory + aKeepAddBufferBefore, 
      aMatchMaxLen + aKeepAddBufferAfter, aWindowReservSize);
    m_SizeHistory = aSizeHistory;
    m_MatchMaxLen = aMatchMaxLen;
    m_HashDescendants = new CDescendant[kHashSize + 1];
    #ifdef __HASH_3
    m_Hash2Descendants = new CDescendant[kHash2Size + 1];
    #endif
    #ifdef __AUTO_REMOVE
    
    #ifdef __HASH_3
    m_NumNodes = aSizeHistory + m_SizeHistory * 4 / 8 + (1 << 19);
    #else
    m_NumNodes = aSizeHistory + m_SizeHistory * 4 / 8 + (1 << 10);
    #endif

    #else
    UINT32 m_NumNodes = aSizeHistory;
    #endif
    const kMaxNumNodes = UINT32(1) << (sizeof(CIndex) * 8 - 1);
    if (m_NumNodes + 32 > kMaxNumNodes)
      return E_INVALIDARG;
    m_Nodes = (CNode *)m_AlignBuffer.Allocate(m_NumNodes + 2, sizeof(CNode), 0x40);
    try 
    {
      delete []m_TmpBacks;
      m_TmpBacks = 0;
      m_TmpBacks = new UINT32[m_MatchMaxLen + 1];
    }
    catch(...)
    {
      m_AlignBuffer.Free();
      throw;
    }
    return S_OK;
  }
  catch(...)
  {
    return E_OUTOFMEMORY;
  }
}

STDMETHODIMP CPatricia::Init(ISequentialInStream *aStream)
{
  RETURN_IF_NOT_S_OK(CIn::Init(aStream));

  // memset(m_HashDescendants, 0xFF, kHashSize * sizeof(m_HashDescendants[0]));

  #ifdef __HASH_3
  for (UINT32 i = 0; i < kHash2Size; i++)
    m_Hash2Descendants[i].MatchPointer = kDescendantsNotInitilized2;
  #else
  for (UINT32 i = 0; i < kHashSize; i++)
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

CPatricia::~CPatricia()
{ 
  delete []m_TmpBacks;
  delete []m_HashDescendants; 
  #ifdef __HASH_3
  delete []m_Hash2Descendants; 
  #endif
}

STDMETHODIMP_(void) CPatricia::ReleaseStream()
{
  CIn::ReleaseStream();
}

// aPos = m_Pos + kNumHashBytes
// aFullCurrentLimit = aCurrentLimit + kNumHashBytes
// aFullMatchLen = aMatchLen + kNumHashBytes

void CPatricia::ChangeLastMatch(UINT32 aHashValue)
{
  UINT32 aPos = m_Pos + kNumHashBytes - 1;
  UINT32 aDescendantIndex;
  const BYTE *aCurrentBytePointer = m_Buffer + aPos;
  UINT32 aNumLoadedBits = 0;
  BYTE aByte;
  CNodePointer aNode = &m_Nodes[m_HashDescendants[aHashValue].NodePointer];

  while(true)
  {
    UINT32 aNumSameBits = aNode->NumSameBits;
    if(aNumSameBits > 0)
    {
      if (aNumLoadedBits < aNumSameBits)
      {
        aNumSameBits -= aNumLoadedBits;
        aCurrentBytePointer += (aNumSameBits / MY_BYTE_SIZE);
        aNumSameBits %= MY_BYTE_SIZE;
        aByte = *aCurrentBytePointer++;
        aNumLoadedBits = MY_BYTE_SIZE; 
      }
      aByte >>= aNumSameBits;
      aNumLoadedBits -= aNumSameBits;
    }
    if(aNumLoadedBits == 0)
    {
      aByte = *aCurrentBytePointer++;
      aNumLoadedBits = MY_BYTE_SIZE; 
    }
    aDescendantIndex = (aByte & kSubNodesMask);
    aNode->LastMatch = aPos;
    aNumLoadedBits -= kNumSubBits;
    aByte >>= kNumSubBits;
    if(aNode->Descendants[aDescendantIndex].IsNode())
      aNode = &m_Nodes[aNode->Descendants[aDescendantIndex].NodePointer];
    else
      break;
  }
  aNode->Descendants[aDescendantIndex].MatchPointer = aPos + kMatchStartValue;
}

UINT32 CPatricia::GetLongestMatch(UINT32 *aBacks)
{
  UINT32 aFullCurrentLimit;
  if (m_Pos + m_MatchMaxLen <= m_StreamPos)
    aFullCurrentLimit = m_MatchMaxLen;
  else
  {
    aFullCurrentLimit = m_StreamPos - m_Pos;
    if(aFullCurrentLimit < kNumHashBytes)
      return 0; 
  }
  UINT32 aPos = m_Pos + kNumHashBytes;

  #ifdef __HASH_3
  UINT32 aHashValueTemp = (*((UINT32 *)(m_Buffer + m_Pos)));
  UINT32 aHashValue = ((aHashValueTemp << 8) | 
      ((aHashValueTemp & 0xFFFFFF)>> 16)) & 0xFFFFFF;
  CDescendant &aHashDescendant = m_HashDescendants[aHashValue];
  CDescendant &aHash2Descendant = m_Hash2Descendants[aHashValueTemp & 0xFFFF];
  if(aHash2Descendant.MatchPointer <= kDescendantEmptyValue2)
  {
    if(aHash2Descendant.MatchPointer == kDescendantsNotInitilized2)
    {
      UINT32 aBase = aHashValue & 0xFFFF00;
      for (UINT32 i = 0; i < 0x100; i++)
        m_HashDescendants[aBase + i].MakeEmpty();
    }
    aHash2Descendant.MatchPointer = aPos + kMatchStartValue2;
    aHashDescendant.MatchPointer = aPos + kMatchStartValue;
    return 0;
  }

  aBacks[kNumHash2Bytes] = aPos - (aHash2Descendant.MatchPointer - kMatchStartValue2) - 1;
  aHash2Descendant.MatchPointer = aPos + kMatchStartValue2;
  #ifdef __AUTO_REMOVE
  if (aBacks[kNumHash2Bytes] >= m_SizeHistory)
  {
    if (aHashDescendant.IsNode())
      RemoveNode(aHashDescendant.NodePointer);
    aHashDescendant.MatchPointer = aPos + kMatchStartValue;
    return 0;
  }
  #endif
  if (aFullCurrentLimit == kNumHash2Bytes)
    return kNumHash2Bytes;

  #else
  UINT32 aHashValue = UINT32(GetIndexByte(1))  | (UINT32(GetIndexByte(0)) << 8);
  CDescendant &aHashDescendant = m_HashDescendants[aHashValue];
  #endif


  if(m_SpecialMode)
  {
    if(aHashDescendant.IsMatch())
      m_NumNotChangedCycles = 0;
    if(m_NumNotChangedCycles >= m_SizeHistory - 1)
    {
      ChangeLastMatch(aHashValue);
      m_NumNotChangedCycles = 0;
    }
    if(GetIndexByte(aFullCurrentLimit - 1) == GetIndexByte(aFullCurrentLimit - 2)) 
    {
      if(aHashDescendant.IsMatch())
        aHashDescendant.MatchPointer = aPos + kMatchStartValue;
      else
        m_NumNotChangedCycles++;
      for(UINT32 i = kNumHashBytes; i <= aFullCurrentLimit; i++)
        aBacks[i] = 0;
      return aFullCurrentLimit;
    }
    else if(m_NumNotChangedCycles > 0)
      ChangeLastMatch(aHashValue);
    m_SpecialMode = false;
  }

  if(aHashDescendant.IsEmpty())
  {
    aHashDescendant.MatchPointer = aPos + kMatchStartValue;
    return kPrevHashSize;
  }

  UINT32 aCurrentLimit = aFullCurrentLimit - kNumHashBytes;

  if(aHashDescendant.IsMatch())
  {
    CMatchPointer aMatchPointer = aHashDescendant.MatchPointer;
    UINT32 aBackReal = aPos - (aMatchPointer - kMatchStartValue);
    UINT32 aBack = aBackReal - 1;
    #ifdef __AUTO_REMOVE
    if (aBack >= m_SizeHistory)
    {
      aHashDescendant.MatchPointer = aPos + kMatchStartValue;
      return kPrevHashSize;
    }
    #endif

    UINT32 aMatchLen;
    aBacks += kNumHashBytes;
    BYTE *aBuffer = m_Buffer + aPos;
    for(aMatchLen = 0; true; aMatchLen++)
    {
      *aBacks++ = aBack;
      if (aMatchLen == aCurrentLimit)
      {
        aHashDescendant.MatchPointer = aPos + kMatchStartValue;
        return kNumHashBytes + aMatchLen;
      }
      if (aBuffer[aMatchLen] != aBuffer[aMatchLen - aBackReal])
        break;
    }
     
    // UINT32 aMatchLen = GetMatchLen(kNumHashBytes, aBack, aCurrentLimit);
    
    UINT32 aFullMatchLen = aMatchLen + kNumHashBytes; 
    aHashDescendant.NodePointer = m_FreeNode;
    CNodePointer aNode = &m_Nodes[m_FreeNode];
    m_FreeNode = aNode->NextFreeNode;
    #ifdef __AUTO_REMOVE
    m_NumUsedNodes++;
    #endif
    if (m_FreeNode > m_FreeNodeMax)
    {
      m_FreeNodeMax = m_FreeNode;
      m_Nodes[m_FreeNode].NextFreeNode = m_FreeNode + 1;
    }
      
    for (UINT32 i = 0; i < kNumSubNodes; i++)
      aNode->Descendants[i].NodePointer = kDescendantEmptyValue;
    aNode->LastMatch = aPos;
      
    BYTE aByteNew = GetIndexByte(aFullMatchLen);
    BYTE aByteOld = GetIndexByte(aFullMatchLen - aBackReal);
    BYTE aBitsNew, aBitsOld;
    UINT32 aNumSameBits = aMatchLen * MY_BYTE_SIZE;
    while (true)
    {
      aBitsNew = (aByteNew & kSubNodesMask);
      aBitsOld = (aByteOld & kSubNodesMask);
      if(aBitsNew != aBitsOld) 
        break;
      aByteNew >>= kNumSubBits;
      aByteOld >>= kNumSubBits;
      aNumSameBits += kNumSubBits;
    }
    aNode->NumSameBits = CSameBitsType(aNumSameBits);
    aNode->Descendants[aBitsNew].MatchPointer = aPos + kMatchStartValue;
    aNode->Descendants[aBitsOld].MatchPointer = aMatchPointer;
    return aFullMatchLen;
  }
  const BYTE *aBaseCurrentBytePointer = m_Buffer + aPos;
  const BYTE *aCurrentBytePointer = aBaseCurrentBytePointer;
  UINT32 aNumLoadedBits = 0;
  BYTE aByte = 0;
  CIndex *aNodePointerPointer = &aHashDescendant.NodePointer;
  CNodePointer aNode = &m_Nodes[*aNodePointerPointer];
  aBacks += kNumHashBytes;
  const BYTE *aBytePointerLimit = aBaseCurrentBytePointer + aCurrentLimit;
  const BYTE *aCurrentAddingOffset = m_Buffer;

  #ifdef __AUTO_REMOVE
  UINT32 aLowPos;
  if (aPos > m_SizeHistory)
    aLowPos = aPos - m_SizeHistory;
  else
    aLowPos = 0;
  #endif

  while(true)
  {
    #ifdef __AUTO_REMOVE
    if (aNode->LastMatch < aLowPos)
    {
      RemoveNode(*aNodePointerPointer);
      *aNodePointerPointer = aPos + kMatchStartValue;
      if (aCurrentBytePointer == aBaseCurrentBytePointer)
        return kPrevHashSize;
      return kNumHashBytes + (aCurrentBytePointer - aBaseCurrentBytePointer - 1);
    }
    #endif
    if(aNumLoadedBits == 0)
    {
      *aBacks++ = aPos - aNode->LastMatch - 1;
      if(aCurrentBytePointer >= aBytePointerLimit)
      {
        for (UINT32 i = 0; i < kNumSubNodes; i++)
          aNode->Descendants[i].MatchPointer = aPos + kMatchStartValue;
        aNode->LastMatch = aPos;
        aNode->NumSameBits = 0;
        return aFullCurrentLimit;
      }
      aByte = (*aCurrentBytePointer++);
      aCurrentAddingOffset++;
      aNumLoadedBits = MY_BYTE_SIZE; 
    }
    UINT32 aNumSameBits = aNode->NumSameBits;
    if(aNumSameBits > 0)
    {
      BYTE aByteXOR = ((*(aCurrentAddingOffset + aNode->LastMatch -1)) >> 
          (MY_BYTE_SIZE - aNumLoadedBits)) ^ aByte;
      while(aNumLoadedBits <= aNumSameBits)
      {
        if(aByteXOR != 0)
        {
          AddInternalNode(aNode, aNodePointerPointer, aByte, aByteXOR,
              aNumSameBits, aPos);
          return kNumHashBytes + (aCurrentBytePointer - aBaseCurrentBytePointer - 1);
        }
        *aBacks++ = aPos - aNode->LastMatch - 1;
        aNumSameBits -= aNumLoadedBits;
        if(aCurrentBytePointer >= aBytePointerLimit)
        {
          for (UINT32 i = 0; i < kNumSubNodes; i++)
            aNode->Descendants[i].MatchPointer = aPos + kMatchStartValue;
          aNode->LastMatch = aPos;
          aNode->NumSameBits = CSameBitsType(aNode->NumSameBits - aNumSameBits);
          return aFullCurrentLimit;
        }
        aNumLoadedBits = MY_BYTE_SIZE; 
        aByte = (*aCurrentBytePointer++);
        aByteXOR = aByte ^ (*(aCurrentAddingOffset + aNode->LastMatch));
        aCurrentAddingOffset++;
      }
      if((aByteXOR & ((1 << aNumSameBits) - 1)) != 0)
      {
        AddInternalNode(aNode, aNodePointerPointer, aByte, aByteXOR,
            aNumSameBits, aPos);
        return kNumHashBytes + (aCurrentBytePointer - aBaseCurrentBytePointer - 1);
      }
      aByte >>= aNumSameBits;
      aNumLoadedBits -= aNumSameBits;
    }
    UINT32 aDescendantIndex = (aByte & kSubNodesMask);
    aNumLoadedBits -= kNumSubBits;
    aNodePointerPointer = &aNode->Descendants[aDescendantIndex].NodePointer;
    UINT32 aNextNodeIndex = *aNodePointerPointer;
    aNode->LastMatch = aPos;
    if (aNextNodeIndex < kDescendantEmptyValue)
    {
      aByte >>= kNumSubBits;
      aNode = &m_Nodes[aNextNodeIndex];
    }
    else if (aNextNodeIndex == kDescendantEmptyValue)
    {
      aNode->Descendants[aDescendantIndex].MatchPointer = aPos + kMatchStartValue;
      return kNumHashBytes + (aCurrentBytePointer - aBaseCurrentBytePointer - 1);
    }
    else 
      break;
  }
 
  UINT32 aDescendantIndex = (aByte & kSubNodesMask);
  aByte >>= kNumSubBits;
  CMatchPointer aMatchPointer = aNode->Descendants[aDescendantIndex].MatchPointer;
  CMatchPointer aRealMatchPointer;
  aRealMatchPointer = aMatchPointer - kMatchStartValue;

  #ifdef __AUTO_REMOVE
  if (aRealMatchPointer < aLowPos)
  {
    aNode->Descendants[aDescendantIndex].MatchPointer = aPos + kMatchStartValue;
    return kNumHashBytes + (aCurrentBytePointer - aBaseCurrentBytePointer - 1);
  }
  #endif

  BYTE aByteXOR;
  UINT32 aNumSameBits = 0;
  if(aNumLoadedBits != 0)
  {
    BYTE aMatchByte = *(aCurrentAddingOffset + aRealMatchPointer -1);  
    aMatchByte >>= (MY_BYTE_SIZE - aNumLoadedBits);
    aByteXOR = aMatchByte ^ aByte;
    if(aByteXOR != 0)
    {
      AddLeafNode(aNode, aByte, aByteXOR, aNumSameBits, aPos, aDescendantIndex);
      return kNumHashBytes + (aCurrentBytePointer - aBaseCurrentBytePointer - 1);
    }
    aNumSameBits += aNumLoadedBits;
  }

  const BYTE *aMatchBytePointer = m_Buffer + aRealMatchPointer + 
      (aCurrentBytePointer - aBaseCurrentBytePointer);
  for(; aCurrentBytePointer < aBytePointerLimit; aNumSameBits += MY_BYTE_SIZE)
  {
    aByte = (*aCurrentBytePointer++);
    *aBacks++ = aPos - aRealMatchPointer - 1;
    aByteXOR = aByte ^ (*aMatchBytePointer++);
    if(aByteXOR != 0)
    {
      AddLeafNode(aNode, aByte, aByteXOR, aNumSameBits, aPos, aDescendantIndex);
      return kNumHashBytes + (aCurrentBytePointer - aBaseCurrentBytePointer - 1);
    }
  }
  *aBacks = aPos - aRealMatchPointer - 1;
  aNode->Descendants[aDescendantIndex].MatchPointer = aPos + kMatchStartValue;

  if(*aBacks == 0)
  {
    m_SpecialMode = true;
    m_NumNotChangedCycles = 0;
  }
  return aFullCurrentLimit;
}

STDMETHODIMP_(void) CPatricia::DummyLongestMatch()
{
  GetLongestMatch(m_TmpBacks);
}


// ------------------------------------
// Remove Match

typedef BYTE CRemoveDataWord;

static const kSizeRemoveDataWordInBits = MY_BYTE_SIZE * sizeof(CRemoveDataWord);

#ifndef __AUTO_REMOVE

void CPatricia::RemoveMatch()
{
  if(m_SpecialRemoveMode)
  {
    if(GetIndexByte(m_MatchMaxLen - 1 - m_SizeHistory) ==
        GetIndexByte(m_MatchMaxLen - m_SizeHistory))
      return;
    m_SpecialRemoveMode = false;
  }
  UINT32 aPos = m_Pos + kNumHashBytes - m_SizeHistory;

  #ifdef __HASH_3
  // UINT32 aHashValue = (*((UINT32 *)(m_Buffer + m_Pos - m_SizeHistory))) & 0xFFFFFF;
  UINT32 aHashValueTemp = *((UINT32 *)(m_Buffer + m_Pos - m_SizeHistory));
  UINT32 aHashValue = ((aHashValueTemp << 8) | 
      ((aHashValueTemp & 0xFFFFFF)>> 16)) & 0xFFFFFF;

  CDescendant &aHashDescendant = m_HashDescendants[aHashValue];
  CDescendant &aHash2Descendant = m_Hash2Descendants[aHashValueTemp & 0xFFFF];
  if (aHash2Descendant >= kMatchStartValue2)
    if(aHash2Descendant.MatchPointer == aPos + kMatchStartValue2)
      aHash2Descendant.MatchPointer = kDescendantEmptyValue2;
  #else
  UINT32 aHashValue = UINT32(GetIndexByte(1 - m_SizeHistory))  | 
      (UINT32(GetIndexByte(0 - m_SizeHistory)) << 8);
  CDescendant &aHashDescendant = m_HashDescendants[aHashValue];
  #endif
    
  if(aHashDescendant.IsEmpty())
    return;
  if(aHashDescendant.IsMatch())
  {
    if(aHashDescendant.MatchPointer == aPos + kMatchStartValue)
      aHashDescendant.MakeEmpty();
    return;
  }
  
  UINT32 aDescendantIndex;
  const CRemoveDataWord *aCurrentPointer = (const CRemoveDataWord *)(m_Buffer + aPos);
  UINT32 aNumLoadedBits = 0;
  CRemoveDataWord aWord;

  CIndex *aNodePointerPointer = &aHashDescendant.NodePointer;

  CNodePointer aNode = &m_Nodes[aHashDescendant.NodePointer];
  
  while(true)
  {
    if(aNumLoadedBits == 0)
    {
      aWord = *aCurrentPointer++;
      aNumLoadedBits = kSizeRemoveDataWordInBits; 
    }
    UINT32 aNumSameBits = aNode->NumSameBits;
    if(aNumSameBits > 0)
    {
      if (aNumLoadedBits <= aNumSameBits)
      {
        aNumSameBits -= aNumLoadedBits;
        aCurrentPointer += (aNumSameBits / kSizeRemoveDataWordInBits);
        aNumSameBits %= kSizeRemoveDataWordInBits;
        aWord = *aCurrentPointer++;
        aNumLoadedBits = kSizeRemoveDataWordInBits; 
      }
      aWord >>= aNumSameBits;
      aNumLoadedBits -= aNumSameBits;
    }
    aDescendantIndex = (aWord & kSubNodesMask);
    aNumLoadedBits -= kNumSubBits;
    aWord >>= kNumSubBits;
    UINT32 aNextNodeIndex = aNode->Descendants[aDescendantIndex].NodePointer;
    if (aNextNodeIndex < kDescendantEmptyValue)
    {
      aNodePointerPointer = &aNode->Descendants[aDescendantIndex].NodePointer;
      aNode = &m_Nodes[aNextNodeIndex];
    }
    else
      break;
  }
  if (aNode->Descendants[aDescendantIndex].MatchPointer != aPos + kMatchStartValue)
  {
    const BYTE *aCurrentBytePointer = m_Buffer + m_Pos - m_SizeHistory;
    const BYTE *aCurrentBytePointerLimit = aCurrentBytePointer + m_MatchMaxLen;
    for(;aCurrentBytePointer < aCurrentBytePointerLimit; aCurrentBytePointer++)
      if(*aCurrentBytePointer != *(aCurrentBytePointer+1))
        return;
    m_SpecialRemoveMode = true;
    return;
  }

  UINT32 aNumNodes = 0, aNumMatches = 0;

  for (UINT32 i = 0; i < kNumSubNodes; i++)
  {
    UINT32 aNodeIndex = aNode->Descendants[i].NodePointer;
    if (aNodeIndex < kDescendantEmptyValue)
      aNumNodes++;
    else if (aNodeIndex > kDescendantEmptyValue)
      aNumMatches++;
  }
  aNumMatches -= 1;
  if (aNumNodes + aNumMatches > 1)
  {
    aNode->Descendants[aDescendantIndex].MakeEmpty();
    return;
  }
  if(aNumNodes == 1)
  {
    UINT32 i;
    for (i = 0; i < kNumSubNodes; i++)
      if (aNode->Descendants[i].IsNode())
        break;
    UINT32 aNextNodeIndex = aNode->Descendants[i].NodePointer;
    CNodePointer aNextNode = &m_Nodes[aNextNodeIndex];
    aNextNode->NumSameBits += aNode->NumSameBits + kNumSubBits;
    *aNode = *aNextNode;

    aNextNode->NextFreeNode = m_FreeNode;
    m_FreeNode = aNextNodeIndex;
    return;
  }
  UINT32 aMatchPointer;
  for (i = 0; i < kNumSubNodes; i++)
    if (aNode->Descendants[i].IsMatch() && i != aDescendantIndex)
    {
      aMatchPointer = aNode->Descendants[i].MatchPointer;
      break;
    }
  aNode->NextFreeNode = m_FreeNode;
  m_FreeNode = *aNodePointerPointer;
  *aNodePointerPointer = aMatchPointer;
}
#endif

const UINT32 kNormalizeStartPos = (UINT32(1) << (kNumBitsInIndex)) - 
    kMatchStartValue - kNumHashBytes - 1;

STDMETHODIMP CPatricia::MovePos()
{
  #ifndef __AUTO_REMOVE
  if(m_Pos >= m_SizeHistory)
    RemoveMatch();
  #endif
  RETURN_IF_NOT_S_OK(CIn::MovePos());
  #ifdef __AUTO_REMOVE
  if (m_NumUsedNodes >= m_NumNodes)
    TestRemoveNodes();
  #endif
  if (m_Pos >= kNormalizeStartPos)
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

void CPatricia::NormalizeDescendant(CDescendant &aDescendant, UINT32 aSubValue)
{
  if (aDescendant.IsEmpty())
    return;
  if (aDescendant.IsMatch())
    aDescendant.MatchPointer = aDescendant.MatchPointer - aSubValue;
  else
  {
    CNode &aNode = m_Nodes[aDescendant.NodePointer];
    aNode.LastMatch = aNode.LastMatch - aSubValue;
    for (UINT32 i = 0; i < kNumSubNodes; i++)
       NormalizeDescendant(aNode.Descendants[i], aSubValue);
  }
}

void CPatricia::Normalize()
{
  UINT32 aSubValue = m_Pos - m_SizeHistory;
  CIn::ReduceOffsets(aSubValue);
  
  #ifdef __HASH_3

  for(UINT32 aHash = 0; aHash < kHash2Size; aHash++)
  {
    CDescendant &aDescendant = m_Hash2Descendants[aHash];
    if (aDescendant.MatchPointer != kDescendantsNotInitilized2)
    {
      UINT32 aBase = aHash << 8;
      for (UINT32 i = 0; i < 0x100; i++)
        NormalizeDescendant(m_HashDescendants[aBase + i], aSubValue);
    }
    if (aDescendant.MatchPointer < kMatchStartValue2)
      continue;
    aDescendant.MatchPointer = aDescendant.MatchPointer - aSubValue;
  }
  
  #else
  
  for(UINT32 aHash = 0; aHash < kHashSize; aHash++)
    NormalizeDescendant(m_HashDescendants[aHash], aSubValue);
  
  #endif

}

#else

void CPatricia::TestRemoveDescendant(CDescendant &aDescendant, UINT32 aLimitPos)
{
  CNode &aNode = m_Nodes[aDescendant.NodePointer];
  UINT32 aNumChilds = 0;
  UINT32 aChildIndex;
  for (UINT32 i = 0; i < kNumSubNodes; i++)
  {
    CDescendant &aDescendant2 = aNode.Descendants[i];
    if (aDescendant2.IsEmpty())
      continue;
    if (aDescendant2.IsMatch())
    {
      if (aDescendant2.MatchPointer < aLimitPos)
        aDescendant2.MakeEmpty();
      else
      {
        aNumChilds++;
        aChildIndex = i;
      }
    }
    else
    {
      TestRemoveDescendant(aDescendant2, aLimitPos);
      if (!aDescendant2.IsEmpty())
      {
        aNumChilds++;
        aChildIndex = i;
      }
    }
  }
  if (aNumChilds > 1)
    return;

  CIndex aNodePointerTemp = aDescendant.NodePointer;
  if (aNumChilds == 1)
  {
    const CDescendant &aDescendant2 = aNode.Descendants[aChildIndex];
    if (aDescendant2.IsNode())
      m_Nodes[aDescendant2.NodePointer].NumSameBits += aNode.NumSameBits + kNumSubBits;
    aDescendant = aDescendant2;
  }
  else
    aDescendant.MakeEmpty();
  aNode.NextFreeNode = m_FreeNode;
  m_FreeNode = aNodePointerTemp;
  m_NumUsedNodes--;
}

void CPatricia::RemoveNode(UINT32 anIndex)
{
  CNode &aNode = m_Nodes[anIndex];
  for (UINT32 i = 0; i < kNumSubNodes; i++)
  {
    CDescendant &aDescendant2 = aNode.Descendants[i];
    if (aDescendant2.IsNode())
      RemoveNode(aDescendant2.NodePointer);
  }
  aNode.NextFreeNode = m_FreeNode;
  m_FreeNode = anIndex;
  m_NumUsedNodes--;
}

void CPatricia::TestRemoveNodes()
{
  UINT32 aLimitPos = kMatchStartValue + m_Pos - m_SizeHistory + kNumHashBytes;
  
  #ifdef __HASH_3
  
  UINT32 aLimitPos2 = kMatchStartValue2 + m_Pos - m_SizeHistory + kNumHashBytes;
  for(UINT32 aHash = 0; aHash < kHash2Size; aHash++)
  {
    CDescendant &aDescendant = m_Hash2Descendants[aHash];
    if (aDescendant.MatchPointer != kDescendantsNotInitilized2)
    {
      UINT32 aBase = aHash << 8;
      for (UINT32 i = 0; i < 0x100; i++)
      {
        CDescendant &aDescendant = m_HashDescendants[aBase + i];
        if (aDescendant.IsEmpty())
          continue;
        if (aDescendant.IsMatch())
        {
          if (aDescendant.MatchPointer < aLimitPos)
            aDescendant.MakeEmpty();
        }
        else
          TestRemoveDescendant(aDescendant, aLimitPos);
      }
    }
    if (aDescendant.MatchPointer < kMatchStartValue2)
      continue;
    if (aDescendant.MatchPointer < aLimitPos2)
      aDescendant.MatchPointer = kDescendantEmptyValue2;
  }
  
  #else
  
  for(UINT32 aHash = 0; aHash < kHashSize; aHash++)
  {
    CDescendant &aDescendant = m_HashDescendants[aHash];
    if (aDescendant.IsEmpty())
      continue;
    if (aDescendant.IsMatch())
    {
      if (aDescendant.MatchPointer < aLimitPos)
        aDescendant.MakeEmpty();
    }
    else
      TestRemoveDescendant(aDescendant, aLimitPos);
  }
  
  #endif
}

void CPatricia::TestRemoveAndNormalizeDescendant(CDescendant &aDescendant, 
    UINT32 aLimitPos, UINT32 aSubValue)
{
  if (aDescendant.IsEmpty())
    return;
  if (aDescendant.IsMatch())
  {
    if (aDescendant.MatchPointer < aLimitPos)
      aDescendant.MakeEmpty();
    else
      aDescendant.MatchPointer = aDescendant.MatchPointer - aSubValue;
    return;
  }
  CNode &aNode = m_Nodes[aDescendant.NodePointer];
  UINT32 aNumChilds = 0;
  UINT32 aChildIndex;
  for (UINT32 i = 0; i < kNumSubNodes; i++)
  {
    CDescendant &aDescendant2 = aNode.Descendants[i];
    TestRemoveAndNormalizeDescendant(aDescendant2, aLimitPos, aSubValue);
    if (!aDescendant2.IsEmpty())
    {
      aNumChilds++;
      aChildIndex = i;
    }
  }
  if (aNumChilds > 1)
  {
    aNode.LastMatch = aNode.LastMatch - aSubValue;
    return;
  }

  CIndex aNodePointerTemp = aDescendant.NodePointer;
  if (aNumChilds == 1)
  {
    const CDescendant &aDescendant2 = aNode.Descendants[aChildIndex];
    if (aDescendant2.IsNode())
      m_Nodes[aDescendant2.NodePointer].NumSameBits += aNode.NumSameBits + kNumSubBits;
    aDescendant = aDescendant2;
  }
  else
    aDescendant.MakeEmpty();
  aNode.NextFreeNode = m_FreeNode;
  m_FreeNode = aNodePointerTemp;
  m_NumUsedNodes--;
}

void CPatricia::TestRemoveNodesAndNormalize()
{
  UINT32 aSubValue = m_Pos - m_SizeHistory;
  UINT32 aLimitPos = kMatchStartValue + m_Pos - m_SizeHistory + kNumHashBytes;
  CIn::ReduceOffsets(aSubValue);

  #ifdef __HASH_3
  
  UINT32 aLimitPos2 = kMatchStartValue2 + m_Pos - m_SizeHistory + kNumHashBytes;
  for(UINT32 aHash = 0; aHash < kHash2Size; aHash++)
  {
    CDescendant &aDescendant = m_Hash2Descendants[aHash];
    if (aDescendant.MatchPointer != kDescendantsNotInitilized2)
    {
      UINT32 aBase = aHash << 8;
      for (UINT32 i = 0; i < 0x100; i++)
        TestRemoveAndNormalizeDescendant(m_HashDescendants[aBase + i], aLimitPos, aSubValue);
    }
    if (aDescendant.MatchPointer < kMatchStartValue2)
      continue;
    if (aDescendant.MatchPointer < aLimitPos2)
      aDescendant.MatchPointer = kDescendantEmptyValue2;
    else
      aDescendant.MatchPointer = aDescendant.MatchPointer - aSubValue;
  }
  
  #else

  for(UINT32 aHash = 0; aHash < kHashSize; aHash++)
    TestRemoveAndNormalizeDescendant(m_HashDescendants[aHash], aLimitPos, aSubValue);

  #endif
}

#endif

STDMETHODIMP_(BYTE) CPatricia::GetIndexByte(UINT32 anIndex)
{
  return CIn::GetIndexByte(anIndex);
}

STDMETHODIMP_(UINT32) CPatricia::GetMatchLen(UINT32 aIndex, UINT32 aBack, UINT32 aLimit)
{
  return CIn::GetMatchLen(aIndex, aBack, aLimit);
}

STDMETHODIMP_(UINT32) CPatricia::GetNumAvailableBytes()
{
  return CIn::GetNumAvailableBytes();
}

STDMETHODIMP_(const BYTE *) CPatricia::GetPointerToCurrentPos()
{
  return CIn::GetPointerToCurrentPos();
}

}
