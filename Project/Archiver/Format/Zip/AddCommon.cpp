// AddCommon.cpp

#include "StdAfx.h"

#include "AddCommon.h"
#include "Archive/Zip/Header.h"
#include "Windows/PropVariant.h"
#include "Windows/Defs.h"
#include "../../../Compress/Interface/CompressInterface.h"

#ifdef COMPRESS_DEFLATE
#include "../../../Compress/LZ/Deflate/Encoder.h"
#else
// {23170F69-40C1-278B-0401-080000000100}
DEFINE_GUID(CLSID_CCompressDeflateEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00);
#endif

namespace NArchive {
namespace NZip {

static const BYTE kMethodIDForEmptyStream = NFileHeader::NCompressionMethod::kStored;
static const BYTE kExtractVersionForEmptyStream = NFileHeader::NCompressionMethod::kStoreExtractVersion;

CAddCommon::CAddCommon(const CCompressionMethodMode &anOptions):
  m_Options(anOptions),
  m_CopyCoderSpec(NULL)
 {}

/*
// {23170F69-40C1-278C-0200-0000000000}
DEFINE_GUID(CLSID_CMatchFinderBT3, 
0x23170F69, 0x40C1, 0x278C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
*/


HRESULT CAddCommon::Compress(IInStream *anInStream, IOutStream *anOutStream, 
      UINT64 anInSize, ICompressProgressInfo *aProgress, CCompressingResult &anOperationResult)
{
  if(anInSize == 0)
  {
    anOperationResult.PackSize = 0;
    anOperationResult.Method = kMethodIDForEmptyStream;
    anOperationResult.ExtractVersion = kExtractVersionForEmptyStream;
    return S_OK;
  }
  int aNumTestMethods = m_Options.MethodSequence.Size();
  BYTE aMethod;
  UINT64 aResultSize = 0;
  for(int i = 0; i < aNumTestMethods; i++)
  {
    anOutStream->Seek(0, STREAM_SEEK_SET, NULL);
    anInStream->Seek(0, STREAM_SEEK_SET, NULL);

    aMethod = m_Options.MethodSequence[i];
    switch(aMethod)
    {
      case NFileHeader::NCompressionMethod::kStored:
      {
        if(m_CopyCoderSpec == NULL)
        {
          m_CopyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
          m_CopyCoder = m_CopyCoderSpec;
        }
        RETURN_IF_NOT_S_OK(m_CopyCoder->Code(anInStream, anOutStream, 
            NULL, NULL, aProgress));
        anOperationResult.ExtractVersion = NFileHeader::NCompressionMethod::kStoreExtractVersion;
        break;
      }
      default:
      {
        if(!m_DeflateEncoder)
        {
          // RETURN_IF_NOT_S_OK(m_MatchFinder.CoCreateInstance(CLSID_CMatchFinderBT3));
          #ifdef COMPRESS_DEFLATE
            m_DeflateEncoder = new CComObjectNoLock<NDeflate::NEncoder::CCoder>;
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
        RETURN_IF_NOT_S_OK(m_DeflateEncoder->Code(anInStream, anOutStream, 
            NULL, NULL, aProgress));
        anOperationResult.ExtractVersion = NFileHeader::NCompressionMethod::kDeflateExtractVersion;
        break;
      }
    }
    anOutStream->Seek(0, STREAM_SEEK_CUR, &aResultSize);
    if(aResultSize < anInSize) 
      break;
  }
  anOutStream->SetSize(aResultSize);
  anOperationResult.PackSize = aResultSize;
  anOperationResult.Method = aMethod;
  return S_OK;
}

}}
