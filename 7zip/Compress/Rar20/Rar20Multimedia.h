// Rar20Multimedia.h
// According to unRAR license,
// this code may not be used to develop a 
// RAR (WinRAR) compatible archiver

#ifndef __RAR20_MULTIMEDIA_H
#define __RAR20_MULTIMEDIA_H

#include "../../../Common/Types.h"

namespace NCompress {
namespace NRar20 {
namespace NMultimedia {

struct CAudioVariables
{
  int K1,K2,K3,K4,K5;
  int D1,D2,D3,D4;
  int LastDelta;
  UInt32 Dif[11];
  UInt32 ByteCount;
  int LastChar;

  void Init();
};

const int kNumChanelsMax = 4;

class CPredictor
{
  CAudioVariables m_AudioVariablesArray[kNumChanelsMax];
  int m_ChannelDelta;
public:
  int CurrentChannel;

  void Init();
  Byte Predict();
  void Update(Byte realValue, int predictedValue);
};

}}}

#endif
