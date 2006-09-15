// BZip2Decoder.cpp

#include "StdAfx.h"

#include "BZip2Decoder.h"

#include "../../../Common/Alloc.h"
#include "../../../Common/Defs.h"
#include "../BWT/Mtf8.h"
#include "BZip2CRC.h"

namespace NCompress {
namespace NBZip2 {

const UInt32 kNumThreadsMax = 4;

static const UInt32 kBufferSize = (1 << 17);

static Int16 kRandNums[512] = { 
   619, 720, 127, 481, 931, 816, 813, 233, 566, 247, 
   985, 724, 205, 454, 863, 491, 741, 242, 949, 214, 
   733, 859, 335, 708, 621, 574, 73, 654, 730, 472, 
   419, 436, 278, 496, 867, 210, 399, 680, 480, 51, 
   878, 465, 811, 169, 869, 675, 611, 697, 867, 561, 
   862, 687, 507, 283, 482, 129, 807, 591, 733, 623, 
   150, 238, 59, 379, 684, 877, 625, 169, 643, 105, 
   170, 607, 520, 932, 727, 476, 693, 425, 174, 647, 
   73, 122, 335, 530, 442, 853, 695, 249, 445, 515, 
   909, 545, 703, 919, 874, 474, 882, 500, 594, 612, 
   641, 801, 220, 162, 819, 984, 589, 513, 495, 799, 
   161, 604, 958, 533, 221, 400, 386, 867, 600, 782, 
   382, 596, 414, 171, 516, 375, 682, 485, 911, 276, 
   98, 553, 163, 354, 666, 933, 424, 341, 533, 870, 
   227, 730, 475, 186, 263, 647, 537, 686, 600, 224, 
   469, 68, 770, 919, 190, 373, 294, 822, 808, 206, 
   184, 943, 795, 384, 383, 461, 404, 758, 839, 887, 
   715, 67, 618, 276, 204, 918, 873, 777, 604, 560, 
   951, 160, 578, 722, 79, 804, 96, 409, 713, 940, 
   652, 934, 970, 447, 318, 353, 859, 672, 112, 785, 
   645, 863, 803, 350, 139, 93, 354, 99, 820, 908, 
   609, 772, 154, 274, 580, 184, 79, 626, 630, 742, 
   653, 282, 762, 623, 680, 81, 927, 626, 789, 125, 
   411, 521, 938, 300, 821, 78, 343, 175, 128, 250, 
   170, 774, 972, 275, 999, 639, 495, 78, 352, 126, 
   857, 956, 358, 619, 580, 124, 737, 594, 701, 612, 
   669, 112, 134, 694, 363, 992, 809, 743, 168, 974, 
   944, 375, 748, 52, 600, 747, 642, 182, 862, 81, 
   344, 805, 988, 739, 511, 655, 814, 334, 249, 515, 
   897, 955, 664, 981, 649, 113, 974, 459, 893, 228, 
   433, 837, 553, 268, 926, 240, 102, 654, 459, 51, 
   686, 754, 806, 760, 493, 403, 415, 394, 687, 700, 
   946, 670, 656, 610, 738, 392, 760, 799, 887, 653, 
   978, 321, 576, 617, 626, 502, 894, 679, 243, 440, 
   680, 879, 194, 572, 640, 724, 926, 56, 204, 700, 
   707, 151, 457, 449, 797, 195, 791, 558, 945, 679, 
   297, 59, 87, 824, 713, 663, 412, 693, 342, 606, 
   134, 108, 571, 364, 631, 212, 174, 643, 304, 329, 
   343, 97, 430, 751, 497, 314, 983, 374, 822, 928, 
   140, 206, 73, 263, 980, 736, 876, 478, 430, 305, 
   170, 514, 364, 692, 829, 82, 855, 953, 676, 246, 
   369, 970, 294, 750, 807, 827, 150, 790, 288, 923, 
   804, 378, 215, 828, 592, 281, 565, 555, 710, 82, 
   896, 831, 547, 261, 524, 462, 293, 465, 502, 56, 
   661, 821, 976, 991, 658, 869, 905, 758, 745, 193, 
   768, 550, 608, 933, 378, 286, 215, 979, 792, 961, 
   61, 688, 793, 644, 986, 403, 106, 366, 905, 644, 
   372, 567, 466, 434, 645, 210, 389, 550, 919, 135, 
   780, 773, 635, 389, 707, 100, 626, 958, 165, 504, 
   920, 176, 193, 713, 857, 265, 203, 50, 668, 108, 
   645, 990, 626, 197, 510, 357, 358, 850, 858, 364, 
   936, 638
};

bool CState::Alloc()
{
  if (tt == 0)
    tt = (UInt32 *)BigAlloc(kBlockSizeMax * sizeof(UInt32));
  return (tt != 0);
}

void CState::Free()
{
  ::BigFree(tt);
  tt = 0;
}

#ifdef COMPRESS_BZIP2_MT
void CState::FinishStream(bool needLeave)
{
  Decoder->StreamWasFinished = true;
  StreamWasFinishedEvent.Set();
  if (needLeave)
    Decoder->CS.Leave();
  Decoder->CanStartWaitingEvent.Lock();
  WaitingWasStartedEvent.Set();
}

DWORD CState::ThreadFunc()
{
  for (;;)
  {
    Decoder->CS.Enter();
    if (Decoder->CloseThreads)
    {
      Decoder->CS.Leave();
      return 0;
    }
    if (Decoder->StreamWasFinished)
    {
      FinishStream(true);
      continue;
    }
    HRESULT res = S_OK;
    bool neadLeave = true;
    try 
    {
      UInt32 blockIndex = Decoder->NextBlockIndex;
      UInt32 nextBlockIndex = blockIndex + 1;
      if (nextBlockIndex == Decoder->NumThreads)
        nextBlockIndex = 0;
      Decoder->NextBlockIndex = nextBlockIndex;

      bool wasFinished;
      UInt32 crc;
      res = Decoder->ReadSignatures(wasFinished, crc);
      if (res != S_OK)
      {
        Decoder->Result = res;
        FinishStream(true);
        continue;
      }
      if (wasFinished)
      {
        Decoder->Result = res;
        FinishStream(true);
        continue;
      }

      res = Decoder->ReadBlock(Decoder->BlockSizeMax, *this);
      UInt64 packSize = Decoder->m_InStream.GetProcessedSize();
      if (res != S_OK)
      {
        Decoder->Result = res;
        FinishStream(true);
        continue;
      }
      neadLeave = false;
      Decoder->CS.Leave();

      DecodeBlock1();

      Decoder->m_States[blockIndex].CanWriteEvent.Lock();

      if (DecodeBlock2(Decoder->m_OutStream) != crc)
      {
        Decoder->Result = S_FALSE;
        FinishStream(neadLeave);
        continue;
      }

      if (Decoder->Progress)
      {
        UInt64 unpackSize = Decoder->m_OutStream.GetProcessedSize();
        res = Decoder->Progress->SetRatioInfo(&packSize, &unpackSize);
      }

      Decoder->m_States[nextBlockIndex].CanWriteEvent.Set();
    }
    catch(const CInBufferException &e)  { res = e.ErrorCode; }
    catch(const COutBufferException &e) { res = e.ErrorCode; }
    catch(...) { res = E_FAIL; }
    if (res != S_OK)
    {
      Decoder->Result = res;
      FinishStream(neadLeave);
      continue;
    }
  }
}

static DWORD WINAPI MFThread(void *threadCoderInfo)
{
  return ((CState *)threadCoderInfo)->ThreadFunc();
}
#endif

UInt32 CDecoder::ReadBits(int numBits) {  return m_InStream.ReadBits(numBits); }
Byte CDecoder::ReadByte() {return (Byte)ReadBits(8); }
bool CDecoder::ReadBit() { return ReadBits(1) != 0; }

UInt32 CDecoder::ReadCRC()
{
  UInt32 crc = 0;
  for (int i = 0; i < 4; i++)
  {
    crc <<= 8;
    crc |= ReadByte();
  }
  return crc;
}

HRESULT CDecoder::ReadBlock(UInt32 blockSizeMax, CState &state)
{
  state.BlockRandomised = ReadBit();
  state.OrigPtr = ReadBits(kNumOrigBits);
  
  // in original code it compares OrigPtr to (UInt32)(10 + blockSizeMax)) : why ?
  if (state.OrigPtr >= blockSizeMax) 
    return S_FALSE;

  CMtf8Decoder mtf;
  int numInUse = 0;
  {
    bool inUse16[16];
    int i;
    for (i = 0; i < 16; i++) 
      inUse16[i] = ReadBit();
    for (i = 0; i < 256; i++)
      if (inUse16[i >> 4])
        if (ReadBit())
          mtf.Buffer[numInUse++] = (Byte)i;
    if (numInUse == 0) 
      return S_FALSE;
    mtf.Init(numInUse);
  }
  int alphaSize = numInUse + 2;

  int numTables = ReadBits(kNumTablesBits);
  if (numTables < kNumTablesMin || numTables > kNumTablesMax)
    return S_FALSE;
  
  UInt32 numSelectors = ReadBits(kNumSelectorsBits);
  if (numSelectors < 1 || numSelectors > kNumSelectorsMax)
    return S_FALSE;

  {
    Byte mtfPos[kNumTablesMax];
    int t = 0;
    do
      mtfPos[t] = (Byte)t;
    while(++t < numTables);
    UInt32 i = 0;
    do
    {
      int j = 0;
      while (ReadBit())
        if (++j >= numTables) 
          return S_FALSE;
      Byte tmp = mtfPos[j];
      for (;j > 0; j--) 
        mtfPos[j] = mtfPos[j - 1]; 
      state.m_Selectors[i] = mtfPos[0] = tmp;
    }
    while(++i < numSelectors);
  }

  int t = 0;
  do
  {
    Byte lens[kMaxAlphaSize];
    int len = (int)ReadBits(kNumLevelsBits);
    int i;
    for (i = 0; i < alphaSize; i++) 
    {
      for (;;)
      {
        if (len < 1 || len > kMaxHuffmanLen) 
          return S_FALSE;
        if (!ReadBit()) 
          break;
        if (ReadBit()) 
          len--;
        else 
          len++; 
      }
      lens[i] = (Byte)len;
    }
    for (; i < kMaxAlphaSize; i++) 
      lens[i] = 0;
    if(!m_HuffmanDecoders[t].SetCodeLengths(lens))
      return S_FALSE;
  }
  while(++t < numTables);

  {
    for (int i = 0; i < 256; i++) 
      state.CharCounters[i] = 0;
  }
  
  UInt32 blockSize = 0;
  {
    UInt32 groupIndex = 0;
    UInt32 groupSize = 0;
    CHuffmanDecoder *huffmanDecoder = 0;
    int runPower = 0;
    UInt32 runCounter = 0;
    
    for (;;)
    {
      if (groupSize == 0) 
      {
        if (groupIndex >= numSelectors)
          return S_FALSE;
        groupSize = kGroupSize;
        huffmanDecoder = &m_HuffmanDecoders[state.m_Selectors[groupIndex++]];
      }
      groupSize--;
        
      UInt32 nextSym = huffmanDecoder->DecodeSymbol(&m_InStream);
      
      if (nextSym < 2) 
      {
        runCounter += ((UInt32)(nextSym + 1) << runPower++); 
        if (blockSizeMax - blockSize < runCounter)
          return S_FALSE;
        continue;
      }
      if (runCounter != 0)
      {
        Byte b = mtf.GetHead();
        state.CharCounters[b] += runCounter;
        do 
          state.tt[blockSize++] = (UInt32)b;
        while(--runCounter != 0);
        runPower = 0;
      } 
      if (nextSym <= (UInt32)numInUse) 
      {
        Byte b = mtf.GetAndMove((int)nextSym - 1);
        if (blockSize >= blockSizeMax) 
          return S_FALSE;
        state.CharCounters[b]++;
        state.tt[blockSize++] = (UInt32)b;
      }
      else if (nextSym == (UInt32)numInUse + 1) 
        break;
      else 
        return S_FALSE;
    }
  }
  if (state.OrigPtr >= blockSize)
    return S_FALSE;
  state.BlockSize = blockSize;
  return S_OK;
}

void CState::DecodeBlock1()
{
  UInt32 *charCounters = this->CharCounters;
  {
    UInt32 sum = 0;
    for (UInt32 i = 0; i < 256; i++) 
    {
      sum += charCounters[i];
      charCounters[i] = sum - charCounters[i];
    }
  }
  
  // Compute the T^(-1) vector
  UInt32 blockSize = this->BlockSize;
  UInt32 i = 0;
  do
    tt[charCounters[tt[i] & 0xFF]++] |= (i << 8);
  while(++i < blockSize);
}

UInt32 CState::DecodeBlock2(COutBuffer &m_OutStream)
{
  UInt32 blockSize = this->BlockSize;

  CBZip2CRC crc;
  
  UInt32 randIndex = 1;
  UInt32 randToGo = kRandNums[0] - 2;
  
  int numReps = 0;

  // it's for speed optimization: prefetch & prevByte_init;
  UInt32 tPos = tt[tt[OrigPtr] >> 8];
  Byte prevByte = (Byte)(tPos & 0xFF);
  
  do
  {
    Byte b = (Byte)(tPos & 0xFF);
    tPos = tt[tPos >> 8];
    
    if (BlockRandomised) 
    {
      if (randToGo == 0) 
      {
        b ^= 1;
        randToGo = kRandNums[randIndex++];
        randIndex &= 0x1FF;
      }
      randToGo--;
    }
    
    if (numReps == kRleModeRepSize)
    {
      for (; b > 0; b--)
      {
        crc.UpdateByte(prevByte);
        m_OutStream.WriteByte(prevByte);
      }
      numReps = 0;
      continue;
    }
    if (prevByte == b)
      numReps++;
    else
    {
      numReps = 1;
      prevByte = b;
    }
    crc.UpdateByte(b);
    m_OutStream.WriteByte(b);
  }
  while(--blockSize != 0);
  return crc.GetDigest();
}

#ifdef COMPRESS_BZIP2_MT
CDecoder::CDecoder():
  m_States(0)
{
  m_NumThreadsPrev = 0;
  NumThreads = 1;
  CS.Enter();
}

CDecoder::~CDecoder()
{
  Free();
}

bool CDecoder::Create()
{
  try 
  { 
    if (m_States != 0 && m_NumThreadsPrev == NumThreads)
      return true;
    Free();
    MtMode = (NumThreads > 1);
    m_NumThreadsPrev = NumThreads;
    m_States = new CState[NumThreads];
    if (m_States == 0)
      return false;
    #ifdef COMPRESS_BZIP2_MT
    for (UInt32 t = 0; t < NumThreads; t++)
    {
      CState &ti = m_States[t];
      ti.Decoder = this;
      if (MtMode)
        if (!ti.Thread.Create(MFThread, &ti))
        {
          NumThreads = t;
          Free();
          return false; 
        }
    }
    #endif
  }
  catch(...) { return false; }
  return true;
}

void CDecoder::Free()
{
  if (!m_States)
    return;
  CloseThreads = true;
  CS.Leave();
  for (UInt32 t = 0; t < NumThreads; t++)
  {
    CState &s = m_States[t];
    if (MtMode)
      s.Thread.Wait();
    s.Free();
  }
  delete []m_States;
  m_States = 0;
}
#endif

HRESULT CDecoder::ReadSignatures(bool &wasFinished, UInt32 &crc)
{
  wasFinished = false;
  Byte s[6];
  for (int i = 0; i < 6; i++)
    s[i] = ReadByte();
  crc = ReadCRC();
  if (s[0] == kFinSig0)
  {
    if (s[1] != kFinSig1 || 
        s[2] != kFinSig2 || 
        s[3] != kFinSig3 || 
        s[4] != kFinSig4 || 
        s[5] != kFinSig5)
      return S_FALSE;
    
    wasFinished = true;
    return (crc == CombinedCRC.GetDigest()) ? S_OK : S_FALSE;
  }
  if (s[0] != kBlockSig0 || 
      s[1] != kBlockSig1 || 
      s[2] != kBlockSig2 || 
      s[3] != kBlockSig3 || 
      s[4] != kBlockSig4 || 
      s[5] != kBlockSig5)
    return S_FALSE;
  CombinedCRC.Update(crc);
  return S_OK;
}

HRESULT CDecoder::DecodeFile(bool &isBZ, ICompressProgressInfo *progress)
{
  #ifdef COMPRESS_BZIP2_MT
  Progress = progress;
  if (!Create())
    return E_FAIL;
  for (UInt32 t = 0; t < NumThreads; t++)
  {
    CState &s = m_States[t];
    if (!s.Alloc())
      return E_OUTOFMEMORY;
    s.StreamWasFinishedEvent.Reset();
    s.WaitingWasStartedEvent.Reset();
    s.CanWriteEvent.Reset();
  }
  #else
  if (!m_States[0].Alloc())
    return E_OUTOFMEMORY;
  #endif

  isBZ = false;
  Byte s[6];
  int i;
  for (i = 0; i < 4; i++)
    s[i] = ReadByte();
  if (s[0] != kArSig0 || 
      s[1] != kArSig1 || 
      s[2] != kArSig2 || 
      s[3] <= kArSig3 || 
      s[3] > kArSig3 + kBlockSizeMultMax)
    return S_OK;
  isBZ = true;
  UInt32 dicSize = (UInt32)(s[3] - kArSig3) * kBlockSizeStep;

  CombinedCRC.Init();
  #ifdef COMPRESS_BZIP2_MT
  if (MtMode)
  {
    NextBlockIndex = 0;
    StreamWasFinished = false;
    CloseThreads = false;
    CanStartWaitingEvent.Reset();
    m_States[0].CanWriteEvent.Set();
    BlockSizeMax = dicSize;
    Result = S_OK;
    CS.Leave();
    UInt32 t;
    for (t = 0; t < NumThreads; t++)
      m_States[t].StreamWasFinishedEvent.Lock();
    CS.Enter();
    CanStartWaitingEvent.Set();
    for (t = 0; t < NumThreads; t++)
      m_States[t].WaitingWasStartedEvent.Lock();
    CanStartWaitingEvent.Reset();
    RINOK(Result);
  }
  else
  #endif
  {
    CState &state = m_States[0];
    for (;;)
    {
      if (progress)
      {
        UInt64 packSize = m_InStream.GetProcessedSize();
        UInt64 unpackSize = m_OutStream.GetProcessedSize();
        RINOK(progress->SetRatioInfo(&packSize, &unpackSize));
      }
      bool wasFinished;
      UInt32 crc;
      RINOK(ReadSignatures(wasFinished, crc));
      if (wasFinished)
        return S_OK;

      RINOK(ReadBlock(dicSize, state));
      state.DecodeBlock1();
      if (state.DecodeBlock2(m_OutStream) != crc)
        return S_FALSE;
    }
  }
  return S_OK;
}

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 * /* inSize */, const UInt64 * /* outSize */,
    ICompressProgressInfo *progress)
{
  if (!m_InStream.Create(kBufferSize))
    return E_OUTOFMEMORY;
  if (!m_OutStream.Create(kBufferSize))
    return E_OUTOFMEMORY;

  m_InStream.SetStream(inStream);
  m_InStream.Init();

  m_OutStream.SetStream(outStream);
  m_OutStream.Init();

  CDecoderFlusher flusher(this);

  bool isBZ;
  RINOK(DecodeFile(isBZ, progress));
  return isBZ ? S_OK: S_FALSE;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(...) { return S_FALSE; }
}

STDMETHODIMP CDecoder::GetInStreamProcessedSize(UInt64 *value)
{
  if (value == NULL)
    return E_INVALIDARG;
  *value = m_InStream.GetProcessedSize();
  return S_OK;
}

#ifdef COMPRESS_BZIP2_MT
STDMETHODIMP CDecoder::SetNumberOfThreads(UInt32 numThreads)
{
  NumThreads = numThreads;
  if (NumThreads < 1)
    NumThreads = 1;
  if (NumThreads > kNumThreadsMax)
    NumThreads = kNumThreadsMax;
  return S_OK;
}
#endif

}}
