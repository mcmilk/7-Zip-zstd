// Crypto/Rar20/Crypto.cpp

#include "StdAfx.h"

#include "Rar20Crypto.h"
#include "Common/Crc.h"

#define  rol(x,n)  (((x)<<(n)) | ((x)>>(8*sizeof(x)-(n))))
#define  ror(x,n)  (((x)>>(n)) | ((x)<<(8*sizeof(x)-(n))))

namespace NCrypto {
namespace NRar20 {

static const int kNumRounds = 32;

static const BYTE InitSubstTable[256]={
  215, 19,149, 35, 73,197,192,205,249, 28, 16,119, 48,221,  2, 42,
  232,  1,177,233, 14, 88,219, 25,223,195,244, 90, 87,239,153,137,
  255,199,147, 70, 92, 66,246, 13,216, 40, 62, 29,217,230, 86,  6,
   71, 24,171,196,101,113,218,123, 93, 91,163,178,202, 67, 44,235,
  107,250, 75,234, 49,167,125,211, 83,114,157,144, 32,193,143, 36,
  158,124,247,187, 89,214,141, 47,121,228, 61,130,213,194,174,251,
   97,110, 54,229,115, 57,152, 94,105,243,212, 55,209,245, 63, 11,
  164,200, 31,156, 81,176,227, 21, 76, 99,139,188,127, 17,248, 51,
  207,120,189,210,  8,226, 41, 72,183,203,135,165,166, 60, 98,  7,
  122, 38,155,170, 69,172,252,238, 39,134, 59,128,236, 27,240, 80,
  131,  3, 85,206,145, 79,154,142,159,220,201,133, 74, 64, 20,129,
  224,185,138,103,173,182, 43, 34,254, 82,198,151,231,180, 58, 10,
  118, 26,102, 12, 50,132, 22,191,136,111,162,179, 45,  4,148,108,
  161, 56, 78,126,242,222, 15,175,146, 23, 33,241,181,190, 77,225,
    0, 46,169,186, 68, 95,237, 65, 53,208,253,168,  9, 18,100, 52,
  116,184,160, 96,109, 37, 30,106,140,104,150,  5,204,117,112, 84
};

void CData::UpdateKeys(const BYTE *data)
{
  for (int i = 0; i < 16; i += 4)
    for (int j = 0; j < 4; j ++)
      Keys[j] ^= CCRC::Table[data[i + j]];
}

static void Swap(BYTE *Ch1, BYTE *Ch2)
{
  BYTE Ch = *Ch1;
  *Ch1 = *Ch2;
  *Ch2 = Ch;
}

void CData::SetPassword(const BYTE *password, UINT32 passwordLength)
{

  // SetOldKeys(password);
  
  Keys[0] = 0xD3A3B879L;
  Keys[1] = 0x3F6D12F7L;
  Keys[2] = 0x7515A235L;
  Keys[3] = 0xA4E7F123L;
  
  BYTE Psw[256];
  memset(Psw, 0, sizeof(Psw));
  
  memmove(Psw, password, passwordLength);
  
  memcpy(SubstTable, InitSubstTable, sizeof(SubstTable));
  for (UINT32 j = 0; j < 256; j++)
    for (UINT32 i = 0; i < passwordLength; i += 2)
    {
      UINT32 n2 = (BYTE)CCRC::Table[(Psw[i + 1] + j) & 0xFF];
      UINT32 n1 = (BYTE)CCRC::Table[(Psw[i] - j) & 0xFF];
      for (UINT32 k = 1; (n1 & 0xFF) != n2; n1++, k++)
        Swap(&SubstTable[n1 & 0xFF], &SubstTable[(n1 + i + k) & 0xFF]);
    }
  for (UINT32 i = 0; i < passwordLength; i+= 16)
    EncryptBlock(&Psw[i]);
}

void CData::EncryptBlock(BYTE *Buf)
{
  UINT32 A, B, C, D, T, TA, TB;

  UINT32 *BufPtr;
  BufPtr = (UINT32 *)Buf;
  
  A = BufPtr[0] ^ Keys[0];
  B = BufPtr[1] ^ Keys[1];
  C = BufPtr[2] ^ Keys[2];
  D = BufPtr[3] ^ Keys[3];

  for(int i = 0; i < kNumRounds; i++)
  {
    T = ((C + rol(D, 11)) ^ Keys[i & 3]);
    TA = A ^ SubstLong(T);
    T=((D ^ rol(C, 17)) + Keys[i & 3]);
    TB = B ^ SubstLong(T);
    A = C;
    B = D;
    C = TA;
    D = TB;
  }

  BufPtr[0] = C ^ Keys[0];
  BufPtr[1] = D ^ Keys[1];
  BufPtr[2] = A ^ Keys[2];
  BufPtr[3] = B ^ Keys[3];

  UpdateKeys(Buf);
}

void CData::DecryptBlock(BYTE *Buf)
{
  BYTE InBuf[16];
  UINT32 A, B, C, D, T, TA, TB;

  UINT32 *BufPtr;
  BufPtr = (UINT32 *)Buf;
  
  A = BufPtr[0] ^ Keys[0];
  B = BufPtr[1] ^ Keys[1];
  C = BufPtr[2] ^ Keys[2];
  D = BufPtr[3] ^ Keys[3];

  memcpy(InBuf, Buf, sizeof(InBuf));
  
  for(int i = kNumRounds - 1; i >= 0; i--)
  {
    T = ((C + rol(D, 11)) ^ Keys[i & 3]);
    TA = A ^ SubstLong(T);
    T = ((D ^ rol(C, 17)) + Keys[i & 3]);
    TB = B ^ SubstLong(T);
    A = C;
    B = D;
    C = TA;
    D = TB;
  }

  BufPtr[0] = C ^ Keys[0];
  BufPtr[1] = D ^ Keys[1];
  BufPtr[2] = A ^ Keys[2];
  BufPtr[3] = B ^ Keys[3];

  UpdateKeys(InBuf);
}

}}
