// AddCommon.cpp

#include "StdAfx.h"

#include "Common/CRC.h"

#include "AddCommon.h"
#include "Archive/Zip/Header.h"
#include "Windows/PropVariant.h"
#include "Windows/Defs.h"
#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"

#ifdef COMPRESS_DEFLATE
#include "../../../Compress/LZ/Deflate/Encoder.h"
#else
// {23170F69-40C1-278B-0401-080000000100}
DEFINE_GUID(CLSID_CCompressDeflateEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00);
#endif

#ifdef COMPRESS_DEFLATE64
#include "../../../Compress/LZ/Deflate/Encoder.h"
#else
// {23170F69-40C1-278B-0401-090000000100}
DEFINE_GUID(CLSID_CCompressDeflate64Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x09, 0x00, 0x00, 0x00, 0x01, 0x00);
#endif


#ifdef CRYPTO_ZIP
#include "../../../Crypto/Cipher/Zip/ZipCipher.h"
#else
// {23170F69-40C1-278B-06F1-0101000000100}
DEFINE_GUID(CLSID_CCryptoZipEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00);
#endif


namespace NArchive {
namespace NZip {

static const BYTE kMethodIDForEmptyStream = NFileHeader::NCompressionMethod::kStored;
static const BYTE kExtractVersionForEmptyStream = NFileHeader::NCompressionMethod::kStoreExtractVersion;

CAddCommon::CAddCommon(const CCompressionMethodMode &options):
  _options(options),
  _copyCoderSpec(NULL),
  _mixerCoderSpec(0)
 {}

static HRESULT GetStreamCRC(IInStream *inStream, UINT32 &resultCRC)
{
  CCRC crc;
  crc.Init();
  const UINT32 kBufferSize = (1 << 14);
  BYTE buffer[kBufferSize];
  while(true)
  {
    UINT32 realProcessedSize;
    RETURN_IF_NOT_S_OK(inStream->Read(buffer, kBufferSize, &realProcessedSize));
    if(realProcessedSize == 0)
    {
      resultCRC = crc.GetDigest();
      return S_OK;
    }
    crc.Update(buffer, realProcessedSize);
  }
}

HRESULT CAddCommon::Compress(IInStream *inStream, IOutStream *outStream, 
      UINT64 inSize, ICompressProgressInfo *progress, CCompressingResult &operationResult)
{
  if(inSize == 0)
  {
    operationResult.PackSize = 0;
    operationResult.Method = kMethodIDForEmptyStream;
    operationResult.ExtractVersion = kExtractVersionForEmptyStream;
    return S_OK;
  }
  int aNumTestMethods = _options.MethodSequence.Size();
  BYTE method;
  UINT64 aResultSize = 0;
  for(int i = 0; i < aNumTestMethods; i++)
  {
    if (_options.PasswordIsDefined)
    {
      if (!_cryptoEncoder)
      {
        #ifdef CRYPTO_ZIP
        _cryptoEncoder = new CComObjectNoLock<NCrypto::NZip::CEncoder>;
        #else
        RETURN_IF_NOT_S_OK(_cryptoEncoder.CoCreateInstance(CLSID_CCryptoZipEncoder));
        #endif
      }
      CComPtr<ICryptoSetPassword> cryptoSetPassword;
      RETURN_IF_NOT_S_OK(_cryptoEncoder.QueryInterface(&cryptoSetPassword));
      RETURN_IF_NOT_S_OK(cryptoSetPassword->CryptoSetPassword(
          (const BYTE *)(const char *)_options.Password, _options.Password.Length()));
      UINT32 crc;
      RETURN_IF_NOT_S_OK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
      RETURN_IF_NOT_S_OK(GetStreamCRC(inStream, crc));
      CComPtr<ICryptoSetCRC> cryptoSetCRC;
      RETURN_IF_NOT_S_OK(_cryptoEncoder.QueryInterface(&cryptoSetCRC));
      RETURN_IF_NOT_S_OK(cryptoSetCRC->CryptoSetCRC(crc));
    }

    RETURN_IF_NOT_S_OK(outStream->Seek(0, STREAM_SEEK_SET, NULL));
    RETURN_IF_NOT_S_OK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    
    method = _options.MethodSequence[i];
    switch(method)
    {
      case NFileHeader::NCompressionMethod::kStored:
      {
        if(_copyCoderSpec == NULL)
        {
          _copyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
          _copyCoder = _copyCoderSpec;
        }
        if (_options.PasswordIsDefined)
        {
          if (!_mixerCoder || _mixerCoderMethod != method)
          {
            _mixerCoder.Release();
            _mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
            _mixerCoder = _mixerCoderSpec;
            _mixerCoderSpec->AddCoder(_copyCoder);
            _mixerCoderSpec->AddCoder(_cryptoEncoder);
            _mixerCoderSpec->FinishAddingCoders();
            _mixerCoderMethod = method;
          }
          _mixerCoderSpec->ReInit();
          _mixerCoderSpec->SetCoderInfo(0, NULL, NULL);
          _mixerCoderSpec->SetCoderInfo(1, NULL, NULL);
          _mixerCoderSpec->SetProgressCoderIndex(0);
          RETURN_IF_NOT_S_OK(_mixerCoder->Code(inStream, outStream,
              NULL, NULL, progress));
        }
        else
        {
          RETURN_IF_NOT_S_OK(_copyCoder->Code(inStream, outStream, 
              NULL, NULL, progress));
        }
        operationResult.ExtractVersion = NFileHeader::NCompressionMethod::kStoreExtractVersion;
        break;
      }
      default:
      {
        if(!_deflateEncoder)
        {
          // RETURN_IF_NOT_S_OK(m_MatchFinder.CoCreateInstance(CLSID_CMatchFinderBT3));
          if (method == NFileHeader::NCompressionMethod::kDeflated64)
          {
            #ifdef COMPRESS_DEFLATE64
              _deflateEncoder = new CComObjectNoLock<NDeflate::NEncoder::CCOMCoder64>;
            #else
            RETURN_IF_NOT_S_OK(_deflateEncoder.CoCreateInstance(CLSID_CCompressDeflate64Encoder));
            #endif
          }
          else
          {
            #ifdef COMPRESS_DEFLATE
              _deflateEncoder = new CComObjectNoLock<NDeflate::NEncoder::CCOMCoder>;
            #else
            RETURN_IF_NOT_S_OK(_deflateEncoder.CoCreateInstance(CLSID_CCompressDeflateEncoder));
            #endif
          }


          /*
          CComPtr<IInitMatchFinder> anInitMatchFinder;
          RETURN_IF_NOT_S_OK(_deflateEncoder->QueryInterface(&anInitMatchFinder));
          anInitMatchFinder->InitMatchFinder(m_MatchFinder);
          */

          NWindows::NCOM::CPropVariant properties[2] = 
          {
            _options.NumPasses, _options.NumFastBytes
          };
          PROPID aPropIDs[2] = 
          {
            NEncodingProperies::kNumPasses,
            NEncodingProperies::kNumFastBytes
          };
          CComPtr<ICompressSetEncoderProperties2> aSetEncoderProperties;
          RETURN_IF_NOT_S_OK(_deflateEncoder.QueryInterface(&aSetEncoderProperties));
          aSetEncoderProperties->SetEncoderProperties2(aPropIDs, properties, 2);
        }
        if (_options.PasswordIsDefined)
        {
          if (!_mixerCoder || _mixerCoderMethod != method)
          {
            _mixerCoder.Release();
            _mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
            _mixerCoder = _mixerCoderSpec;
            _mixerCoderSpec->AddCoder(_deflateEncoder);
            _mixerCoderSpec->AddCoder(_cryptoEncoder);
            _mixerCoderSpec->FinishAddingCoders();
            _mixerCoderMethod = method;
          }
          _mixerCoderSpec->ReInit();
          _mixerCoderSpec->SetCoderInfo(0, NULL, NULL);
          _mixerCoderSpec->SetCoderInfo(1, NULL, NULL);
          _mixerCoderSpec->SetProgressCoderIndex(0);
          RETURN_IF_NOT_S_OK(_mixerCoder->Code(inStream, outStream,
              NULL, NULL, progress));
        }
        else
        {
          RETURN_IF_NOT_S_OK(_deflateEncoder->Code(inStream, outStream, 
            NULL, NULL, progress));
        }
        operationResult.ExtractVersion = NFileHeader::NCompressionMethod::kDeflateExtractVersion;
        break;
      }
    }
    outStream->Seek(0, STREAM_SEEK_CUR, &aResultSize);
    if(aResultSize < inSize) 
      break;
  }
  outStream->SetSize(aResultSize);
  operationResult.PackSize = aResultSize;
  operationResult.Method = method;
  return S_OK;
}

}}
