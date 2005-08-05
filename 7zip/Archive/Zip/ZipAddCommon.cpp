// AddCommon.cpp

#include "StdAfx.h"

#include "Common/CRC.h"
#include "Windows/PropVariant.h"
#include "Windows/Defs.h"
#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../7z/7zMethods.h"

#include "ZipAddCommon.h"
#include "ZipHeader.h"

#ifdef COMPRESS_DEFLATE
#include "../../Compress/Deflate/DeflateEncoder.h"
#else
// {23170F69-40C1-278B-0401-080000000100}
DEFINE_GUID(CLSID_CCompressDeflateEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00);
#endif

#ifdef COMPRESS_DEFLATE64
#include "../../Compress/Deflate/DeflateEncoder.h"
#else
// {23170F69-40C1-278B-0401-090000000100}
DEFINE_GUID(CLSID_CCompressDeflate64Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x09, 0x00, 0x00, 0x00, 0x01, 0x00);
#endif

#ifdef COMPRESS_BZIP2
#include "../../Compress/BZip2/BZip2Encoder.h"
#else
// {23170F69-40C1-278B-0402-020000000100}
DEFINE_GUID(CLSID_CCompressBZip2Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00);
#endif


#ifdef CRYPTO_ZIP
#include "../../Crypto/Zip/ZipCipher.h"
#else
// {23170F69-40C1-278B-06F1-0101000000100}
DEFINE_GUID(CLSID_CCryptoZipEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00);
#endif

namespace NArchive {
namespace NZip {

static const Byte kMethodIDForEmptyStream = NFileHeader::NCompressionMethod::kStored;
static const Byte kExtractVersionForEmptyStream = NFileHeader::NCompressionMethod::kStoreExtractVersion;

CAddCommon::CAddCommon(const CCompressionMethodMode &options):
  _options(options),
  _copyCoderSpec(NULL),
  _cryptoStreamSpec(0)
 {}

static HRESULT GetStreamCRC(IInStream *inStream, UInt32 &resultCRC)
{
  CCRC crc;
  crc.Init();
  const UInt32 kBufferSize = (1 << 14);
  Byte buffer[kBufferSize];
  while(true)
  {
    UInt32 realProcessedSize;
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
      UInt64 inSize, ICompressProgressInfo *progress, CCompressingResult &operationResult)
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
  Byte method;
  UInt64 resultSize = 0;
  COutStreamReleaser outStreamReleaser;
  for(int i = 0; i < numTestMethods; i++)
  {
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    RINOK(outStream->Seek(0, STREAM_SEEK_SET, NULL));
    if (_options.PasswordIsDefined)
    {
      if (!_cryptoStream)
      {
        _cryptoStreamSpec = new CFilterCoder;
        _cryptoStream = _cryptoStreamSpec;
        _filterSpec = new NCrypto::NZip::CEncoder;
        _cryptoStreamSpec->Filter = _filterSpec;
      }
      RINOK(_filterSpec->CryptoSetPassword(
          (const Byte *)(const char *)_options.Password, _options.Password.Length()));
      UInt32 crc;
      RINOK(GetStreamCRC(inStream, crc));
      RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
      RINOK(_cryptoStreamSpec->SetOutStream(outStream));
      outStreamReleaser.FilterCoder = _cryptoStreamSpec;
      RINOK(_filterSpec->CryptoSetCRC(crc));
      RINOK(_filterSpec->WriteHeader(outStream));
    }

    method = _options.MethodSequence[i];
    switch(method)
    {
      case NFileHeader::NCompressionMethod::kStored:
      {
        if(_copyCoderSpec == NULL)
        {
          _copyCoderSpec = new NCompress::CCopyCoder;
          _copyCoder = _copyCoderSpec;
        }
        CMyComPtr<ISequentialOutStream> outStreamNew;
        if (_options.PasswordIsDefined)
          outStreamNew = _cryptoStream;
        else
          outStreamNew = outStream;
        RINOK(_copyCoder->Code(inStream, outStreamNew, 
              NULL, NULL, progress));
        operationResult.ExtractVersion = NFileHeader::NCompressionMethod::kStoreExtractVersion;
        break;
      }
      default:
      {
        if(!_compressEncoder)
        {
          // RINOK(m_MatchFinder.CoCreateInstance(CLSID_CMatchFinderBT3));
          #ifndef COMPRESS_DEFLATE
          UString methodName;
          N7z::LoadMethodMap();
          #endif
          switch(method)
          {
            case NFileHeader::NCompressionMethod::kDeflated:
            {
              #ifdef COMPRESS_DEFLATE
              _compressEncoder = new NCompress::NDeflate::NEncoder::CCOMCoder;
              #else
              methodName = L"Deflate";
              #endif
              break;
            }
            case NFileHeader::NCompressionMethod::kDeflated64:
            {
              #ifdef COMPRESS_DEFLATE64
              _compressEncoder = new NCompress::NDeflate::NEncoder::CCOMCoder64;
              #else
              methodName = L"Deflate64";
              #endif
              break;
            }
            case NFileHeader::NCompressionMethod::kBZip2:
            {
              #ifdef COMPRESS_BZIP2
              _compressEncoder = new NCompress::NBZip2::CEncoder;
              #else
              methodName = L"BZip2";
              #endif
              break;
            }
          }
          #ifndef COMPRESS_DEFLATE
          N7z::CMethodInfo2 methodInfo;
          if (!N7z::GetMethodInfo(methodName, methodInfo))
            return E_NOTIMPL;
          RINOK(_compressLib.LoadAndCreateCoder(
            methodInfo.FilePath, methodInfo.Encoder, &_compressEncoder));
          #endif

          if (method == NFileHeader::NCompressionMethod::kDeflated ||
              method == NFileHeader::NCompressionMethod::kDeflated64)
          {
            NWindows::NCOM::CPropVariant properties[2] = 
            {
              _options.NumPasses, 
              _options.NumFastBytes
            };
            PROPID propIDs[2] = 
            {
              NCoderPropID::kNumPasses,
              NCoderPropID::kNumFastBytes
            };
            CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
            RINOK(_compressEncoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProperties));
            RINOK(setCoderProperties->SetCoderProperties(propIDs, properties, 2));
          } else if (method == NFileHeader::NCompressionMethod::kBZip2)
          {
            NWindows::NCOM::CPropVariant properties[1] = 
            {
              _options.NumPasses
            };
            PROPID propIDs[1] = 
            {
              NCoderPropID::kNumPasses,
            };
            CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
            RINOK(_compressEncoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProperties));
            RINOK(setCoderProperties->SetCoderProperties(propIDs, properties, 1));
          }

        }
        CMyComPtr<ISequentialOutStream> outStreamNew;
        if (_options.PasswordIsDefined)
          outStreamNew = _cryptoStream;
        else
          outStreamNew = outStream;
        RINOK(_compressEncoder->Code(inStream, outStreamNew, NULL, NULL, progress));
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
  RINOK(outStream->SetSize(resultSize));
  operationResult.PackSize = resultSize;
  operationResult.Method = method;
  return S_OK;
}

}}
