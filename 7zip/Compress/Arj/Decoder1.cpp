// Arj/Decoder.cpp

#include "StdAfx.h"

#include "Decoder1.h"

#include "Windows/Defs.h"

namespace NCompress{
namespace NArj {
namespace NDecoder1 {

static const UINT32 kHistorySize = 26624;
static const UINT32 kMatchMaxLen = 256;
static const UINT32 kMatchMinLen = 3;

static const UINT32 kNC = 255 + kMatchMaxLen + 2 - kMatchMinLen;

CCoder::CCoder()
{}

void CCoder::make_table(int nchar, BYTE *bitlen, int tablebits, 
    UINT32 *table, int tablesize)
{
  UINT32 count[17], weight[17], start[18], *p;
  UINT32 i, k, len, ch, jutbits, avail, nextcode, mask;
  
  for (i = 1; i <= 16; i++)
    count[i] = 0;
  for (i = 0; (int)i < nchar; i++)
    count[bitlen[i]]++;
  
  start[1] = 0;
  for (i = 1; i <= 16; i++)
    start[i + 1] = start[i] + (count[i] << (16 - i));
  if (start[17] != (UINT32) (1 << 16))
    throw "Data error";
  
  jutbits = 16 - tablebits;
  for (i = 1; (int)i <= tablebits; i++)
  {
    start[i] >>= jutbits;
    weight[i] = 1 << (tablebits - i);
  }
  while (i <= 16)
  {
    weight[i] = 1 << (16 - i);
    i++;
  }
  
  i = start[tablebits + 1] >> jutbits;
  if (i != (UINT32) (1 << 16))
  {
    k = 1 << tablebits;
    while (i != k)
      table[i++] = 0;
  }
  
  avail = nchar;
  mask = 1 << (15 - tablebits);
  for (ch = 0; (int)ch < nchar; ch++)
  {
    if ((len = bitlen[ch]) == 0)
      continue;
    k = start[len];
    nextcode = k + weight[len];
    if ((int)len <= tablebits)
    {
      if (nextcode > (UINT32)tablesize)
        throw "Data error";
      for (i = start[len]; i < nextcode; i++)
        table[i] = ch;
    }
    else
    {
      p = &table[k >> jutbits];
      i = len - tablebits;
      while (i != 0)
      {
        if (*p == 0)
        {
          right[avail] = left[avail] = 0;
          *p = avail++;
        }
        if (k & mask)
          p = &right[*p];
        else
          p = &left[*p];
        k <<= 1;
        i--;
      }
      *p = ch;
    }
    start[len] = nextcode;
  }
}

void CCoder::read_pt_len(int nn, int nbit, int i_special)
{
  UINT32 n = m_InBitStream.ReadBits(nbit);
  if (n == 0)
  {
    UINT32 c = m_InBitStream.ReadBits(nbit);
    int i;
    for (i = 0; i < nn; i++)
      pt_len[i] = 0;
    for (i = 0; i < 256; i++)
      pt_table[i] = c;
  }
  else
  {
    UINT32 i = 0;
    while (i < n)
    {
      UINT32 bitBuf = m_InBitStream.GetValue(16);
      int c = bitBuf >> 13;
      if (c == 7)
      {
        UINT32 mask = 1 << (12);
        while (mask & bitBuf)
        {
          mask >>= 1;
          c++;
        }
      }
      m_InBitStream.MovePos((c < 7) ? 3 : (int)(c - 3));
      pt_len[i++] = (BYTE)c;
      if (i == (UINT32)i_special)
      {
        c = m_InBitStream.ReadBits(2);
        while (--c >= 0)
          pt_len[i++] = 0;
      }
    }
    while (i < (UINT32)nn)
      pt_len[i++] = 0;
    make_table(nn, pt_len, 8, pt_table, PTABLESIZE);
  }
}

void CCoder::read_c_len()
{
  int i, c, n;
  UINT32 mask;
  
  n = m_InBitStream.ReadBits(CBIT);
  if (n == 0)
  {
    c = m_InBitStream.ReadBits(CBIT);
    for (i = 0; i < NC; i++)
      c_len[i] = 0;
    for (i = 0; i < CTABLESIZE; i++)
      c_table[i] = c;
  }
  else
  {
    i = 0;
    while (i < n)
    {
      UINT32 bitBuf = m_InBitStream.GetValue(16);
      c = pt_table[bitBuf >> (8)];
      if (c >= NT)
      {
        mask = 1 << (7);
        do
        {
          if (bitBuf & mask)
            c = right[c];
          else
            c = left[c];
          mask >>= 1;
        } while (c >= NT);
      }
      m_InBitStream.MovePos((int)(pt_len[c]));
      if (c <= 2)
      {
        if (c == 0)
          c = 1;
        else if (c == 1)
          c = m_InBitStream.ReadBits(4) + 3;
        else
          c = m_InBitStream.ReadBits(CBIT) + 20;
        while (--c >= 0)
          c_len[i++] = 0;
      }
      else
        c_len[i++] = (BYTE)(c - 2);
    }
    while (i < NC)
      c_len[i++] = 0;
    make_table(NC, c_len, 12, c_table, CTABLESIZE);
  }
}

UINT32 CCoder::decode_c()
{
  UINT32 j, mask;
  UINT32 bitbuf = m_InBitStream.GetValue(16);
  j = c_table[bitbuf >> 4];
  if (j >= NC)
  {
    mask = 1 << (3);
    do
    {
      if (bitbuf & mask)
        j = right[j];
      else
        j = left[j];
      mask >>= 1;
    } while (j >= NC);
  }
  m_InBitStream.MovePos((int)(c_len[j]));
  return j;
}

UINT32 CCoder::decode_p()
{
  UINT32 j, mask;
  UINT32 bitbuf = m_InBitStream.GetValue(16);
  j = pt_table[bitbuf >> (8)];
  if (j >= NP)
  {
    mask = 1 << (7);
    do
    {
      if (bitbuf & mask)
        j = right[j];
      else
        j = left[j];
      mask >>= 1;
    } while (j >= NP);
  }
  m_InBitStream.MovePos((int)(pt_len[j]));
  if (j != 0)
  {
    j--;
    j = (1 << j) + m_InBitStream.ReadBits((int)j);
  }
  return j;
}


STDMETHODIMP CCoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  int size1 = sizeof(c_table) / sizeof(c_table[0]);
  for (int i = 0; i < size1; i++)
  {
    if (i % 100 == 0)
      c_table[i] = 0;

    c_table[i] = 0;
  }

  if (outSize == NULL)
    return E_INVALIDARG;

  if (!m_OutWindowStream.IsCreated())
  {
    try { m_OutWindowStream.Create(kHistorySize); }
    catch(...) { return E_OUTOFMEMORY; }
  }
  UINT64 pos = 0;
  m_OutWindowStream.Init(outStream, false);
  m_InBitStream.Init(inStream);
  CCoderReleaser coderReleaser(this);

  UINT32 blockSize = 0;

  while(pos < *outSize)
  {
    if (blockSize == 0)
    {
      if (progress != NULL)
      {
        UINT64 packSize = m_InBitStream.GetProcessedSize();
        RINOK(progress->SetRatioInfo(&packSize, &pos));
      }
      blockSize = m_InBitStream.ReadBits(16);
      read_pt_len(NT, TBIT, 3);
      read_c_len();
      read_pt_len(NP, PBIT, -1);
    }
    blockSize--;

    UINT32 number = decode_c();
    if (number < 256)
    {
      m_OutWindowStream.PutOneByte(number);
      pos++;
      continue;
    }
    else
    {
      UINT32 len = number - 256 + kMatchMinLen;
      UINT32 distance = decode_p();
      if (distance >= pos)
        throw "data error";
      m_OutWindowStream.CopyBackBlock(distance, len);
        pos += len;
    }
  }
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress);}
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLZOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

}}}
