// Compression/HuffmanEncoder.cpp

#include "StdAfx.h"

#include "Compression/HuffmanEncoder.h"
#include "Common/Defs.h"

namespace NCompression {
namespace NHuffman {

static const char *kIncorrectBitLenCountsMessage = "Incorrect bit len counts";

CEncoder::CEncoder(UINT32 aNumSymbols,
    const BYTE *anExtraBits, UINT32 anExtraBase, UINT32 aMaxLength):
  m_NumSymbols(aNumSymbols),
  m_ExtraBits(anExtraBits),
  m_ExtraBase(anExtraBase),
  m_MaxLength(aMaxLength),
  m_HeapSize(aNumSymbols * 2+ 1)
{
  m_Items = new CItem[m_HeapSize];
  m_Heap = new UINT32[m_HeapSize];
  m_Depth = new BYTE[m_HeapSize];
}

CEncoder::~CEncoder()
{
  delete []m_Depth;
  delete []m_Heap;
  delete []m_Items;
}

void CEncoder::StartNewBlock()
{
  for (UINT32 i = 0; i < m_NumSymbols; i++)
    m_Items[i].Freq = 0;
}

void CEncoder::SetFreqs(const UINT32 *aFreqs)
{
  for (UINT32 i = 0; i < m_NumSymbols; i++)
    m_Items[i].Freq = aFreqs[i];
}

static const kSmallest = 1;

// ===========================================================================
// Remove the smallest element from the heap and recreate the heap with
// one less element. Updates heap and m_HeapLength.
 
UINT32 CEncoder::RemoveSmallest()
{
  UINT32 aTop = m_Heap[kSmallest]; 
  m_Heap[kSmallest] = m_Heap[m_HeapLength--]; 
  DownHeap(kSmallest); 
  return aTop;
}

// ===========================================================================
// Compares to subtrees, using the tree m_Depth as tie breaker when
// the subtrees have equal frequency. This minimizes the worst case length.

bool CEncoder::Smaller(int n, int m) 
{
  return (m_Items[n].Freq < m_Items[m].Freq || 
         (m_Items[n].Freq == m_Items[m].Freq && m_Depth[n] <= m_Depth[m]));
}

// ===========================================================================
// Restore the m_Heap property by moving down the tree starting at node k,
// exchanging a node with the smallest of its two sons if necessary, stopping
// when the m_Heap property is re-established (each father CompareFreqs than its
// two sons).

void CEncoder::DownHeap(UINT32 k)
{
  UINT32 aSymbol = m_Heap[k];
  for (UINT32 j = k << 1; j <= m_HeapLength;)   // j: left son of k 
  {
    // Set j to the smallest of the two sons: 
    if (j < m_HeapLength && Smaller(m_Heap[j+1], m_Heap[j])) 
      j++;
    UINT32 htemp = m_Heap[j];    // htemp required because of bug in SASC compiler
    if (Smaller(aSymbol, htemp)) // Exit if v is smaller than both sons 
      break;
    m_Heap[k] = htemp;     // Exchange v with the smallest son
    k = j;
    j <<= 1; // And continue down the tree, setting j to the left son of k
  }
  m_Heap[k] = aSymbol;
}

// ===========================================================================
// Compute the optimal bit lengths for a tree and update the total bit length
// for the current block.
// IN assertion: the fields freq and dad are set, heap[aHeapMax] and
//    above are the tree nodes sorted by increasing frequency.
// OUT assertions: the field len is set to the optimal bit length, the
//    array m_BitLenCounters contains the frequencies for each bit length.
//    The length m_BlockBitLength is updated; static_len is also updated if stree is
//    not null.

void CEncoder::GenerateBitLen(UINT32 aMaxCode, UINT32 aHeapMax)
{
  int anOverflow = 0;   // number of elements with bit length too large 
  
  for (UINT32 i = 0; i <= kNumBitsInLongestCode; i++) 
    m_BitLenCounters[i] = 0;
  
  /* In a first pass, compute the optimal bit lengths (which may
  * anOverflow in the case of the bit length tree).
  */
  m_Items[m_Heap[aHeapMax]].Len = 0; /* root of the heap */
  UINT32 h;              /* heap index */
  for (h = aHeapMax+1; h < m_HeapSize; h++) 
  {
    UINT32 aSymbol = m_Heap[h];
    UINT32 aLen = m_Items[m_Items[aSymbol].Dad].Len + 1;
    if (aLen > m_MaxLength) 
    {
      aLen = m_MaxLength;
      anOverflow++;
    }
    m_Items[aSymbol].Len = aLen;  // We overwrite m_Items[aSymbol].Dad which is no longer needed
    if (aSymbol > aMaxCode) 
      continue;                       // not a leaf node
    m_BitLenCounters[aLen]++;
    UINT32 anExtraBits;
    if (m_ExtraBits != 0 && aSymbol >= m_ExtraBase) 
      anExtraBits = m_ExtraBits[aSymbol - m_ExtraBase];
    else
      anExtraBits = 0;
    m_BlockBitLength += (m_Items[aSymbol].Freq * (aLen + anExtraBits));
  }
  if (anOverflow == 0) 
    return;
 
  // This happens for example on obj2 and pic of the Calgary corpus
  // Find the first bit length which could increase:
  do 
  {
    UINT32 aBits = m_MaxLength-1;
    while (m_BitLenCounters[aBits] == 0) 
      aBits--;
    m_BitLenCounters[aBits]--;        // move one leaf down the m_Items
    m_BitLenCounters[aBits + 1] += 2; // move one anOverflow item as its brother
    m_BitLenCounters[m_MaxLength]--;
    // The brother of the anOverflow item also moves one step up,
    // but this does not affect m_BitLenCounters[m_MaxLength]
    anOverflow -= 2;
  } 
  while (anOverflow > 0);
  
  // Now recompute all bit lengths, scanning in increasing frequency.
  // h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
  // lengths instead of fixing only the wrong ones. This idea is taken
  // from 'ar' written by Haruhiko Okumura.)
  for (UINT32 aBits = m_MaxLength; aBits != 0; aBits--) 
  {
    UINT32 aNumNodes = m_BitLenCounters[aBits];
    while (aNumNodes != 0) 
    {
      UINT32 m = m_Heap[--h];
      if (m > aMaxCode) 
        continue;
      if (m_Items[m].Len != (unsigned) aBits) 
      {
        m_BlockBitLength += ((long)aBits - (long)m_Items[m].Len) * (long)m_Items[m].Freq;
        m_Items[m].Len = aBits;
      }
      aNumNodes--;
    }
  }
}


// ===========================================================================
// Generate the codes for a given tree and bit counts (which need not be
// optimal).
// IN assertion: the array m_BitLenCounters contains the bit length statistics for
// the given tree and the field len is set for all tree elements.
// OUT assertion: the field code is set for all tree elements of non
//     zero code length.

//    UINT32 aMaxCode =  largest code with non zero frequency


void CEncoder::GenerateCodes(UINT32 aMaxCode)
{
  UINT32 aNextCodes[kNumBitsInLongestCode + 1]; // next code value for each bit length
  UINT32 code = 0;                        // running code value
                             // The distribution counts are first used to generate the code values
                             // without bit reversal.
  for (UINT32 aBits = 1; aBits <= kNumBitsInLongestCode; aBits++) 
    aNextCodes[aBits] = code = (code + m_BitLenCounters[aBits - 1]) << 1;
  // Check that the bit counts in m_BitLenCounters are consistent. The last code
  // must be all ones.
  if (code + m_BitLenCounters[kNumBitsInLongestCode] - 1 != (1 << kNumBitsInLongestCode) - 1)
    throw kIncorrectBitLenCountsMessage;
  for (UINT32 n = 0;  n <= aMaxCode; n++) 
  {
    int aLen = m_Items[n].Len;
    if (aLen == 0) 
      continue;
    m_Items[n].Code = aNextCodes[aLen]++;
  }
}


// ===========================================================================
// Construct one Huffman tree and assigns the code bit strings and lengths.
// Update the total bit length for the current block.
// IN assertion: the field freq is set for all tree elements.
// OUT assertions: the fields len and code are set to the optimal bit length
//     and corresponding code. The length m_BlockBitLength is updated; static_len is
//     also updated if stree is not null. The field max_code is set.

void CEncoder::BuildTree(BYTE *aLevels)
{
  m_BlockBitLength = 0;
  int aMaxCode = -1; // WAS = -1; largest code with non zero frequency */

  // Construct the initial m_Heap, with least frequent element in
  // m_Heap[kSmallest]. The sons of m_Heap[n] are m_Heap[2*n] and m_Heap[2*n+1].
  //   m_Heap[0] is not used.
  //

  m_HeapLength = 0;
  UINT32 n;   // iterate over m_Heap elements 
  for (n = 0; n < m_NumSymbols; n++) 
  {
    if (m_Items[n].Freq != 0) 
    {
      m_Heap[++m_HeapLength] = aMaxCode = n;
      m_Depth[n] = 0;
    } 
    else 
      m_Items[n].Len = 0;
  }

  // The pkzip format requires that at least one distance code exists,
  // and that at least one bit should be sent even if there is only one
  // possible code. So to avoid special checks later on we force at least
  // two codes of non zero frequency.
  while (m_HeapLength < 2) 
  {
    int aNewNode = m_Heap[++m_HeapLength] = (aMaxCode < 2 ? ++aMaxCode : 0);
    m_Items[aNewNode].Freq = 1;
    m_Depth[aNewNode] = 0;
    m_BlockBitLength--; 
    // if (stree) static_len -= stree[aNewNode].Len;
    //    aNewNode is 0 or 1 so it does not have m_ExtraBits bits
  }
  
  // The elements m_Heap[m_HeapLength/2+1 .. m_HeapLength] are leaves of the m_Items,
  // establish sub-heaps of increasing lengths:
  for (n = m_HeapLength / 2; n >= 1; n--) 
    DownHeap(n);
  
  // Construct the Huffman tree by repeatedly combining the least two
  // frequent nodes.
  int aNode = m_NumSymbols;   // next internal node of the tree
  UINT32 aHeapMax = m_NumSymbols * 2+ 1;
  do 
  {
    n = RemoveSmallest();   /* n = node of least frequency */
    UINT32 m = m_Heap[kSmallest];  /* m = node of next least frequency */
    
    m_Heap[--aHeapMax] = n; /* keep the nodes sorted by frequency */
    m_Heap[--aHeapMax] = m;
    
    // Create a new node father of n and m 
    m_Items[aNode].Freq = m_Items[n].Freq + m_Items[m].Freq;
    m_Depth[aNode] = (BYTE) (MyMax(m_Depth[n], m_Depth[m]) + 1);
    m_Items[n].Dad = m_Items[m].Dad = aNode;
    // and insert the new node in the m_Heap
    m_Heap[kSmallest] = aNode++;
    DownHeap(kSmallest);
    
  } 
  while (m_HeapLength >= 2);
  
  m_Heap[--aHeapMax] = m_Heap[kSmallest];
  
  // At this point, the fields freq and dad are set. We can now
  // generate the bit lengths.
  GenerateBitLen(aMaxCode, aHeapMax);
  
  // The field len is now set, we can generate the bit codes 
  GenerateCodes (aMaxCode);

  for (n = 0; n < m_NumSymbols; n++) 
    aLevels[n] = BYTE(m_Items[n].Len);
}

}}
