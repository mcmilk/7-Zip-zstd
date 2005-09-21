// Rar20Multimedia.cpp
// According to unRAR license,
// this code may not be used to develop a 
// RAR (WinRAR) compatible archiver

#include "StdAfx.h"

#include "Rar20Multimedia.h"

namespace NCompress {
namespace NRar20 {
namespace NMultimedia {

void CAudioVariables::Init()
{
  memset(this, 0, sizeof(CAudioVariables));
}

void CPredictor::Init()
{
  for(int i = 0; i < kNumChanelsMax; i++)
    m_AudioVariablesArray[i].Init();
  m_ChannelDelta = 0;
  CurrentChannel = 0;
}

Byte CPredictor::Predict()
{
  CAudioVariables *v = &m_AudioVariablesArray[CurrentChannel];
  v->ByteCount++;
  v->D4 = v->D3;
  v->D3 = v->D2;
  v->D2 = v->LastDelta-v->D1;
  v->D1 = v->LastDelta;
  int pCh = 8 * v->LastChar + 
            v->K1 * v->D1 + 
            v->K2 * v->D2 + 
            v->K3 * v->D3 + 
            v->K4 * v->D4 + 
            v->K5*m_ChannelDelta;
  pCh = (pCh >> 3) & 0xFF;
  return Byte(pCh);
}

void CPredictor::Update(Byte realValue, int predictedValue)
{
  struct CAudioVariables *v = &m_AudioVariablesArray[CurrentChannel];

  int delta = predictedValue - realValue;
  int i = ((signed char)delta) << 3;

  v->Dif[0] += abs(i);
  v->Dif[1] += abs(i - v->D1);
  v->Dif[2] += abs(i + v->D1);
  v->Dif[3] += abs(i - v->D2);
  v->Dif[4] += abs(i + v->D2);
  v->Dif[5] += abs(i - v->D3);
  v->Dif[6] += abs(i + v->D3);
  v->Dif[7] += abs(i - v->D4);
  v->Dif[8] += abs(i + v->D4);
  v->Dif[9] += abs(i - m_ChannelDelta);
  v->Dif[10] += abs(i + m_ChannelDelta);

  m_ChannelDelta = v->LastDelta = (signed char)(realValue - v->LastChar);
  v->LastChar = realValue;

  UInt32 numMinDif, minDif;
  if ((v->ByteCount & 0x1F)==0)
  {
    minDif = v->Dif[0];
    numMinDif = 0;
    v->Dif[0] = 0;
    for (i = 1; i < sizeof(v->Dif) / sizeof(v->Dif[0]); i++)
    {
      if (v->Dif[i] < minDif)
      {
        minDif = v->Dif[i];
        numMinDif = i;
      }
      v->Dif[i] = 0;
    }
    switch(numMinDif)
    {
      case 1:
        if (v->K1 >= -16)
          v->K1--;
        break;
      case 2:
        if (v->K1 < 16)
          v->K1++;
        break;
      case 3:
        if (v->K2 >= -16)
          v->K2--;
        break;
      case 4:
        if (v->K2 < 16)
          v->K2++;
        break;
      case 5:
        if (v->K3 >= -16)
          v->K3--;
        break;
      case 6:
        if (v->K3 < 16)
          v->K3++;
        break;
      case 7:
        if (v->K4 >= -16)
          v->K4--;
        break;
      case 8:
        if (v->K4 < 16)
          v->K4++;
        break;
      case 9:
        if (v->K5 >= -16)
          v->K5--;
        break;
      case 10:
        if (v->K5 < 16)
          v->K5++;
        break;
    }
  }
}

}}}
