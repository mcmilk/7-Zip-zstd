// Compression/HuffmanEncoder.cpp

#include "StdAfx.h"

#include "HuffmanEncoder.h"
#include "Common/Defs.h"

namespace NCompression {
namespace NHuffman {

static const char *kIncorrectBitLenCountsMessage = "Incorrect bit len counts";

CEncoder::CEncoder(UINT32 numSymbols,
    const BYTE *extraBits, UINT32 extraBase, UINT32 maxLength):
  m_NumSymbols(numSymbols),
  m_ExtraBits(extraBits),
  m_ExtraBase(extraBase),
  m_MaxLength(maxLength),
  m_HeapSize(numSymbols * 2+ 1)
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

void CEncoder::SetFreqs(const UINT32 *freqs)
{
  for (UINT32 i = 0; i < m_NumSymbols; i++)
    m_Items[i].Freq = freqs[i];
}

static const int kSmallest = 1;

// ===========================================================================
// Remove the smallest element from the heap and recreate the heap with
// one less element. Updates heap and m_HeapLength.
 
UINT32 CEncoder::RemoveSmallest()
{
  UINT32 top = m_Heap[kSmallest]; 
  m_Heap[kSmallest] = m_Heap[m_HeapLength--]; 
  DownHeap(kSmallest); 
  return top;
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
  UINT32 symbol = m_Heap[k];
  for (UINT32 j = k << 1; j <= m_HeapLength;)   // j: left son of k 
  {
    // Set j to the smallest of the two sons: 
    if (j < m_HeapLength && Smaller(m_Heap[j+1], m_Heap[j])) 
      j++;
    UINT32 htemp = m_Heap[j];    // htemp required because of bug in SASC compiler
    if (Smaller(symbol, htemp)) // Exit if v is smaller than both sons 
      break;
    m_Heap[k] = htemp;     // Exchange v with the smallest son
    k = j;
    j <<= 1; // And continue down the tree, setting j to the left son of k
  }
  m_Heap[k] = symbol;
}

// ===========================================================================
// Compute the optimal bit lengths for a tree and update the total bit length
// for the current block.
// IN assertion: the fields freq and dad are set, heap[heapMax] and
//    above are the tree nodes sorted by increasing frequency.
// OUT assertions: the field len is set to the optimal bit length, the
//    array m_BitLenCounters contains the frequencies for each bit length.
//    The length m_BlockBitLength is updated; static_len is also updated if stree is
//    not null.

void CEncoder::GenerateBitLen(UINT32 maxCode, UINT32 heapMax)
{
  int overflow = 0;   // number of elements with bit length too large 
  
  for (UINT32 i = 0; i <= kNumBitsInLongestCode; i++) 
    m_BitLenCounters[i] = 0;
  
  /* In a first pass, compute the optimal bit lengths (which may
  * overflow in the case of the bit length tree).
  */
  m_Items[m_Heap[heapMax]].Len = 0; /* root of the heap */
  UINT32 h;              /* heap index */
  for (h = heapMax+1; h < m_HeapSize; h++) 
  {
    UINT32 symbol = m_Heap[h];
    UINT32 len = m_Items[m_Items[symbol].Dad].Len + 1;
    if (len > m_MaxLength) 
    {
      len = m_MaxLength;
      overflow++;
    }
    m_Items[symbol].Len = len;  // We overwrite m_Items[symbol].Dad which is no longer needed
    if (symbol > maxCode) 
      continue;                       // not a leaf node
    m_BitLenCounters[len]++;
    UINT32 extraBits;
    if (m_ExtraBits != 0 && symbol >= m_ExtraBase) 
      extraBits = m_ExtraBits[symbol - m_ExtraBase];
    else
      extraBits = 0;
    m_BlockBitLength += (m_Items[symbol].Freq * (len + extraBits));
  }
  if (overflow == 0) 
    return;
 
  // This happens for example on obj2 and pic of the Calgary corpus
  // Find the first bit length which could increase:
  do 
  {
    UINT32 bits = m_MaxLength-1;
    while (m_BitLenCounters[bits] == 0) 
      bits--;
    m_BitLenCounters[bits]--;        // move one leaf down the m_Items
    m_BitLenCounters[bits + 1] += 2; // move one overflow item as its brother
    m_BitLenCounters[m_MaxLength]--;
    // The brother of the overflow item also moves one step up,
    // but this does not affect m_BitLenCounters[m_MaxLength]
    overflow -= 2;
  } 
  while (overflow > 0);
  
  // Now recompute all bit lengths, scanning in increasing frequency.
  // h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
  // lengths instead of fixing only the wrong ones. This idea is taken
  // from 'ar' written by Haruhiko Okumura.)
  for (UINT32 bits = m_MaxLength; bits != 0; bits--) 
  {
    UINT32 numNodes = m_BitLenCounters[bits];
    while (numNodes != 0) 
    {
      UINT32 m = m_Heap[--h];
      if (m > maxCode) 
        continue;
      if (m_Items[m].Len != (unsigned) bits) 
      {
        m_BlockBitLength += ((long)bits - (long)m_Items[m].Len) * (long)m_Items[m].Freq;
        m_Items[m].Len = bits;
      }
      numNodes--;
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

//    UINT32 maxCode =  largest code with non zero frequency


void CEncoder::GenerateCodes(UINT32 maxCode)
{
  UINT32 nextCodes[kNumBitsInLongestCode + 1]; // next code value for each bit length
  UINT32 code = 0;                        // running code value
                             // The distribution counts are first used to generate the code values
                             // without bit reversal.
  for (UINT32 bits = 1; bits <= kNumBitsInLongestCode; bits++) 
    nextCodes[bits] = code = (code + m_BitLenCounters[bits - 1]) << 1;
  // Check that the bit counts in m_BitLenCounters are consistent. The last code
  // must be all ones.
  if (code + m_BitLenCounters[kNumBitsInLongestCode] - 1 != (1 << kNumBitsInLongestCode) - 1)
    throw kIncorrectBitLenCountsMessage;
  for (UINT32 n = 0;  n <= maxCode; n++) 
  {
    int len = m_Items[n].Len;
    if (len == 0) 
      continue;
    m_Items[n].Code = nextCodes[len]++;
  }
}


// ===========================================================================
// Construct one Huffman tree and assigns the code bit strings and lengths.
// Update the total bit length for the current block.
// IN assertion: the field freq is set for all tree elements.
// OUT assertions: the fields len and code are set to the optimal bit length
//     and corresponding code. The length m_BlockBitLength is updated; static_len is
//     also updated if stree is not null. The field max_code is set.

void CEncoder::BuildTree(BYTE *levels)
{
  m_BlockBitLength = 0;
  int maxCode = -1; // WAS = -1; largest code with non zero frequency */

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
      m_Heap[++m_HeapLength] = maxCode = n;
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
    int aNewNode = m_Heap[++m_HeapLength] = (maxCode < 2 ? ++maxCode : 0);
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
  int node = m_NumSymbols;   // next internal node of the tree
  UINT32 heapMax = m_NumSymbols * 2+ 1;
  do 
  {
    n = RemoveSmallest();   /* n = node of least frequency */
    UINT32 m = m_Heap[kSmallest];  /* m = node of next least frequency */
    
    m_Heap[--heapMax] = n; /* keep the nodes sorted by frequency */
    m_Heap[--heapMax] = m;
    
    // Create a new node father of n and m 
    m_Items[node].Freq = m_Items[n].Freq + m_Items[m].Freq;
    m_Depth[node] = (BYTE) (MyMax(m_Depth[n], m_Depth[m]) + 1);
    m_Items[n].Dad = m_Items[m].Dad = node;
    // and insert the new node in the m_Heap
    m_Heap[kSmallest] = node++;
    DownHeap(kSmallest);
    
  } 
  while (m_HeapLength >= 2);
  
  m_Heap[--heapMax] = m_Heap[kSmallest];
  
  // At this point, the fields freq and dad are set. We can now
  // generate the bit lengths.
  GenerateBitLen(maxCode, heapMax);
  
  // The field len is now set, we can generate the bit codes 
  GenerateCodes (maxCode);

  for (n = 0; n < m_NumSymbols; n++) 
    levels[n] = BYTE(m_Items[n].Len);
}

}}
