// AddCommon.cpp

#include "StdAfx.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"

#include "AddCommon.h"

#ifdef COMPRESS_DEFLATE
#include "../../../Compress/LZ/Deflate/Encoder.h"
#else
// {23170F69-40C1-278B-0401-080000000100}
DEFINE_GUID(CLSID_CCompressDeflateEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00);
#endif

namespace NArchive {
namespace NGZip {

/*
// {23170F69-40C1-278C-0200-0000000000}
DEFINE_GUID(CLSID_CMatchFinderBT3, 
0x23170F69, 0x40C1, 0x278C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
*/

HRESULT CAddCommon::Compress(ISequentialInStream *anInStream, 
    ISequentialOutStream *anOutStream, ICompressProgressInfo *aProgress)
{
  if (!m_DeflateEncoder)
  {
    // RETURN_IF_NOT_S_OK(m_MatchFinder.CoCreateInstance(CLSID_CMatchFinderBT3));
    #ifdef COMPRESS_DEFLATE
    m_DeflateEncoder = new CComObjectNoLock<NDeflate::NEncoder::CCOMCoder>;
    #else
    RETURN_IF_NOT_S_OK(m_DeflateEncoder.CoCreateInstance(CLSID_CCompressDeflateEncoder));
    #endif
    
    /*
    CComPtr<IInitMatchFinder> anInitMatchFinder;
    RETURN_IF_NOT_S_OK(m_DeflateEncoder->QueryInterface(&anInitMatchFinder));
    anInitMatchFinder->InitMatchFinder(m_MatchFinder);
    */

    NWindows::NCOM::CPropVariant aProperties[2] = 
    {
      m_Options.NumPasses, m_Options.NumFastBytes
    };
    PROPID aPropIDs[2] = 
    {
      NEncodingProperies::kNumPasses,
        NEncodingProperies::kNumFastBytes
    };
    CComPtr<ICompressSetEncoderProperties2> aSetEncoderProperties;
    RETURN_IF_NOT_S_OK(m_DeflateEncoder.QueryInterface(&aSetEncoderProperties));
    aSetEncoderProperties->SetEncoderProperties2(aPropIDs, aProperties, 2);
  }
  return m_DeflateEncoder->Code(anInStream, anOutStream, NULL, NULL, aProgress);
}

}}