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

#ifdef COMPRESS_BZIP2
#include "../../../Compress/BWT/BZip2/Encoder.h"
#else
// {23170F69-40C1-278B-0402-020000000100}
DEFINE_GUID(CLSID_CCompressBZip2Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00);
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
    RINOK(inStream->Read(buffer, kBufferSize, &realProcessedSize));
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
  /*
  if(inSize == 0)
  {
    operationResult.PackSize = 0;
    operationResult.Method = kMethodIDForEmptyStream;
    operationResult.ExtractVersion = kExtractVersionForEmptyStream;
    return S_OK;
  }
  */
  int numTestMethods = _options.MethodSequence.Size();
  BYTE method;
  UINT64 resultSize = 0;
  for(int i = 0; i < numTestMethods; i++)
  {
    if (_options.PasswordIsDefined)
    {
      if (!_cryptoEncoder)
      {
        #ifdef CRYPTO_ZIP
        _cryptoEncoder = new CComObjectNoLock<NCrypto::NZip::CEncoder>;
        #else
        RINOK(_cryptoEncoder.CoCreateInstance(CLSID_CCryptoZipEncoder));
        #endif
      }
      CComPtr<ICryptoSetPassword> cryptoSetPassword;
      RINOK(_cryptoEncoder.QueryInterface(&cryptoSetPassword));
      RINOK(cryptoSetPassword->CryptoSetPassword(
          (const BYTE *)(const char *)_options.Password, _options.Password.Length()));
      UINT32 crc;
      RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
      RINOK(GetStreamCRC(inStream, crc));
      CComPtr<ICryptoSetCRC> cryptoSetCRC;
      RINOK(_cryptoEncoder.QueryInterface(&cryptoSetCRC));
      RINOK(cryptoSetCRC->CryptoSetCRC(crc));
    }

    RINOK(outStream->Seek(0, STREAM_SEEK_SET, NULL));
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    
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
          RINOK(_mixerCoder->Code(inStream, outStream,
              NULL, NULL, progress));
        }
        else
        {
          RINOK(_copyCoder->Code(inStream, outStream, 
              NULL, NULL, progress));
        }
        operationResult.ExtractVersion = NFileHeader::NCompressionMethod::kStoreExtractVersion;
        break;
      }
      default:
      {
        if(!_compressEncoder)
        {
          // RINOK(m_MatchFinder.CoCreateInstance(CLSID_CMatchFinderBT3));
          switch(method)
          {
            case NFileHeader::NCompressionMethod::kDeflated:
            {
              #ifdef COMPRESS_DEFLATE
              _compressEncoder = new CComObjectNoLock<NDeflate::NEncoder::CCOMCoder>;
              #else
              RINOK(_compressEncoder.CoCreateInstance(CLSID_CCompressDeflateEncoder));
              #endif
              break;
            }
            case NFileHeader::NCompressionMethod::kDeflated64:
            {
              #ifdef COMPRESS_DEFLATE64
              _compressEncoder = new CComObjectNoLock<NDeflate::NEncoder::CCOMCoder64>;
              #else
              RINOK(_compressEncoder.CoCreateInstance(CLSID_CCompressDeflate64Encoder));
              #endif
              break;
            }
            case NFileHeader::NCompressionMethod::kBZip2:
            {
              #ifdef COMPRESS_BZIP2
              _compressEncoder = new CComObjectNoLock<NCompress::NBZip2::NEncoder::CCoder>;
              #else
              RINOK(_compressEncoder.CoCreateInstance(CLSID_CCompressBZip2Encoder));
              #endif
              break;
            }
          }

          if (method == NFileHeader::NCompressionMethod::kDeflated ||
              method == NFileHeader::NCompressionMethod::kDeflated64)
          {
            NWindows::NCOM::CPropVariant properties[2] = 
            {
              _options.NumPasses, _options.NumFastBytes
            };
            PROPID propIDs[2] = 
            {
              NEncodingProperies::kNumPasses,
              NEncodingProperies::kNumFastBytes
            };
            CComPtr<ICompressSetEncoderProperties2> setEncoderProperties;
            RINOK(_compressEncoder.QueryInterface(&setEncoderProperties));
            setEncoderProperties->SetEncoderProperties2(propIDs, properties, 2);
          }
        }
        if (_options.PasswordIsDefined)
        {
          if (!_mixerCoder || _mixerCoderMethod != method)
          {
            _mixerCoder.Release();
            _mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
            _mixerCoder = _mixerCoderSpec;
            _mixerCoderSpec->AddCoder(_compressEncoder);
            _mixerCoderSpec->AddCoder(_cryptoEncoder);
            _mixerCoderSpec->FinishAddingCoders();
            _mixerCoderMethod = method;
          }
          _mixerCoderSpec->ReInit();
          _mixerCoderSpec->SetCoderInfo(0, NULL, NULL);
          _mixerCoderSpec->SetCoderInfo(1, NULL, NULL);
          _mixerCoderSpec->SetProgressCoderIndex(0);
          RINOK(_mixerCoder->Code(inStream, outStream,
              NULL, NULL, progress));
        }
        else
        {
          RINOK(_compressEncoder->Code(inStream, outStream, NULL, NULL, progress));
        }
        operationResult.ExtractVersion = NFileHeader::NCompressionMethod::kDeflateExtractVersion;
        break;
      }
    }
    outStream->Seek(0, STREAM_SEEK_CUR, &resultSize);
    if (_options.PasswordIsDefined)
    {
      if(resultSize < inSize  + 12) 
        break;
    }
    else if(resultSize < inSize) 
      break;
  }
  outStream->SetSize(resultSize);
  operationResult.PackSize = resultSize;
  operationResult.Method = method;
  return S_OK;
}

}}
