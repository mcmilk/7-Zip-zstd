// ICoder.h

#pragma once

#ifndef __ICODER_H
#define __ICODER_H

#include "IInOutStreams.h"

// {23170F69-40C1-278A-0000-000200040000}
DEFINE_GUID(IID_ICompressProgressInfo, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200040000")
ICompressProgressInfo: public IUnknown
{
  STDMETHOD(SetRatioInfo)(const UINT64 *anInSize, const UINT64 *anOutSize) = 0;
};

// {23170F69-40C1-278A-0000-000200050000}
DEFINE_GUID(IID_ICompressCoder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000200050000")
ICompressCoder: public IUnknown
{
  // STDMETHOD(Create)(UINT32 aHistorySize, UINT32 aMatchFastLen) = 0;
  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress) = 0;
};

#endif
