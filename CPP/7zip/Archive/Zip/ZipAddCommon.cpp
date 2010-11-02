// ZipAddCommon.cpp

#include "StdAfx.h"

#include "../../../../C/7zCrc.h"

#include "Windows/PropVariant.h"

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../MyVersion.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/StreamUtils.h"

#include "../../Compress/LzmaEncoder.h"
#include "../../Compress/PpmdZip.h"

#include "../Common/InStreamWithCRC.h"

#include "ZipAddCommon.h"
#include "ZipHeader.h"

namespace NArchive {
namespace NZip {

static const CMethodId kMethodId_ZipBase   = 0x040100;
static const CMethodId kMethodId_BZip2     = 0x040202;

static const UInt32 kLzmaPropsSize = 5;
static const UInt32 kLzmaHeaderSize = 4 + kLzmaPropsSize;

class CLzmaEncoder:
  public ICompressCoder,
  public CMyUnknownImp
{
  NCompress::NLzma::CEncoder *EncoderSpec;
  CMyComPtr<ICompressCoder> Encoder;
  Byte Header[kLzmaHeaderSize];
public:
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  HRESULT SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);

  MY_UNKNOWN_IMP
};

HRESULT CLzmaEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps)
{
  if (!Encoder)
  {
    EncoderSpec = new NCompress::NLzma::CEncoder;
    Encoder = EncoderSpec;
  }
  CBufPtrSeqOutStream *outStreamSpec = new CBufPtrSeqOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->Init(Header + 4, kLzmaPropsSize);
  RINOK(EncoderSpec->SetCoderProperties(propIDs, props, numProps));
  RINOK(EncoderSpec->WriteCoderProperties(outStream));
  if (outStreamSpec->GetPos() != kLzmaPropsSize)
    return E_FAIL;
  Header[0] = MY_VER_MAJOR;
  Header[1] = MY_VER_MINOR;
  Header[2] = kLzmaPropsSize;
  Header[3] = 0;
  return S_OK;
}

HRESULT CLzmaEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  RINOK(WriteStream(outStream, Header, kLzmaHeaderSize));
  return Encoder->Code(inStream, outStream, inSize, outSize, progress);
}


CAddCommon::CAddCommon(const CCompressionMethodMode &options):
  _options(options),
  _copyCoderSpec(NULL),
  _cryptoStreamSpec(0)
  {}

static HRESULT GetStreamCRC(ISequentialInStream *inStream, UInt32 &resultCRC)
{
  UInt32 crc = CRC_INIT_VAL;
  const UInt32 kBufferSize = (1 << 14);
  Byte buffer[kBufferSize];
  for (;;)
  {
    UInt32 realProcessedSize;
    RINOK(inStream->Read(buffer, kBufferSize, &realProcessedSize));
    if (realProcessedSize == 0)
    {
      resultCRC = CRC_GET_DIGEST(crc);
      return S_OK;
    }
    crc = CrcUpdate(crc, buffer, (size_t)realProcessedSize);
  }
}

HRESULT CAddCommon::Compress(
    DECL_EXTERNAL_CODECS_LOC_VARS
    ISequentialInStream *inStream, IOutStream *outStream,
    ICompressProgressInfo *progress, CCompressingResult &opRes)
{
  CSequentialInStreamWithCRC *inSecCrcStreamSpec = 0;
  CInStreamWithCRC *inCrcStreamSpec = 0;
  CMyComPtr<ISequentialInStream> inCrcStream;
  {
    CMyComPtr<IInStream> inStream2;
    // we don't support stdin, since stream from stdin can require 64-bit size header
    RINOK(inStream->QueryInterface(IID_IInStream, (void **)&inStream2));
    if (inStream2)
    {
      inCrcStreamSpec = new CInStreamWithCRC;
      inCrcStream = inCrcStreamSpec;
      inCrcStreamSpec->SetStream(inStream2);
      inCrcStreamSpec->Init();
    }
    else
    {
      inSecCrcStreamSpec = new CSequentialInStreamWithCRC;
      inCrcStream = inSecCrcStreamSpec;
      inSecCrcStreamSpec->SetStream(inStream);
      inSecCrcStreamSpec->Init();
    }
  }

  int numTestMethods = _options.MethodSequence.Size();
  if (numTestMethods > 1 || _options.PasswordIsDefined)
  {
    if (inCrcStreamSpec == 0)
    {
      if (_options.PasswordIsDefined)
        return E_NOTIMPL;
      numTestMethods = 1;
    }
  }
  Byte method = 0;
  COutStreamReleaser outStreamReleaser;
  opRes.ExtractVersion = NFileHeader::NCompressionMethod::kExtractVersion_Default;
  for (int i = 0; i < numTestMethods; i++)
  {
    opRes.ExtractVersion = NFileHeader::NCompressionMethod::kExtractVersion_Default;
    if (inCrcStreamSpec != 0)
      RINOK(inCrcStreamSpec->Seek(0, STREAM_SEEK_SET, NULL));
    RINOK(outStream->SetSize(0));
    RINOK(outStream->Seek(0, STREAM_SEEK_SET, NULL));
    if (_options.PasswordIsDefined)
    {
      opRes.ExtractVersion = NFileHeader::NCompressionMethod::kExtractVersion_ZipCrypto;

      if (!_cryptoStream)
      {
        _cryptoStreamSpec = new CFilterCoder;
        _cryptoStream = _cryptoStreamSpec;
      }
      if (_options.IsAesMode)
      {
        opRes.ExtractVersion = NFileHeader::NCompressionMethod::kExtractVersion_Aes;
        if (!_cryptoStreamSpec->Filter)
        {
          _cryptoStreamSpec->Filter = _filterAesSpec = new NCrypto::NWzAes::CEncoder;
          _filterAesSpec->SetKeyMode(_options.AesKeyMode);
          RINOK(_filterAesSpec->CryptoSetPassword((const Byte *)(const char *)_options.Password, _options.Password.Length()));
        }
        RINOK(_filterAesSpec->WriteHeader(outStream));
      }
      else
      {
        if (!_cryptoStreamSpec->Filter)
        {
          _cryptoStreamSpec->Filter = _filterSpec = new NCrypto::NZip::CEncoder;
          _filterSpec->CryptoSetPassword((const Byte *)(const char *)_options.Password, _options.Password.Length());
        }
        UInt32 crc = 0;
        RINOK(GetStreamCRC(inStream, crc));
        RINOK(inCrcStreamSpec->Seek(0, STREAM_SEEK_SET, NULL));
        RINOK(_filterSpec->WriteHeader(outStream, crc));
      }
      RINOK(_cryptoStreamSpec->SetOutStream(outStream));
      outStreamReleaser.FilterCoder = _cryptoStreamSpec;
    }

    method = _options.MethodSequence[i];
    switch(method)
    {
      case NFileHeader::NCompressionMethod::kStored:
      {
        if (_copyCoderSpec == NULL)
        {
          _copyCoderSpec = new NCompress::CCopyCoder;
          _copyCoder = _copyCoderSpec;
        }
        CMyComPtr<ISequentialOutStream> outStreamNew;
        if (_options.PasswordIsDefined)
          outStreamNew = _cryptoStream;
        else
          outStreamNew = outStream;
        RINOK(_copyCoder->Code(inCrcStream, outStreamNew, NULL, NULL, progress));
        break;
      }
      default:
      {
        if (!_compressEncoder)
        {
          if (method == NFileHeader::NCompressionMethod::kLZMA)
          {
            _compressExtractVersion = NFileHeader::NCompressionMethod::kExtractVersion_LZMA;
            CLzmaEncoder *_lzmaEncoder = new CLzmaEncoder();
            _compressEncoder = _lzmaEncoder;
            NWindows::NCOM::CPropVariant props[] =
            {
              #ifndef _7ZIP_ST
              _options.NumThreads,
              #endif
              _options.Algo,
              _options.DicSize,
              _options.NumFastBytes,
              const_cast<BSTR>((const wchar_t *)_options.MatchFinder),
              _options.NumMatchFinderCycles
            };
            PROPID propIDs[] =
            {
              #ifndef _7ZIP_ST
              NCoderPropID::kNumThreads,
              #endif
              NCoderPropID::kAlgorithm,
              NCoderPropID::kDictionarySize,
              NCoderPropID::kNumFastBytes,
              NCoderPropID::kMatchFinder,
              NCoderPropID::kMatchFinderCycles
            };
            int numProps = sizeof(propIDs) / sizeof(propIDs[0]);
            if (!_options.NumMatchFinderCyclesDefined)
              numProps--;
            RINOK(_lzmaEncoder->SetCoderProperties(propIDs, props, numProps));
          }
          else if (method == NFileHeader::NCompressionMethod::kPPMd)
          {
            _compressExtractVersion = NFileHeader::NCompressionMethod::kExtractVersion_PPMd;
            NCompress::NPpmdZip::CEncoder *encoder = new NCompress::NPpmdZip::CEncoder();
            _compressEncoder = encoder;
            NWindows::NCOM::CPropVariant props[] =
            {
              _options.Algo,
              _options.MemSize,
              _options.Order
              
            };
            PROPID propIDs[] =
            {
              NCoderPropID::kAlgorithm,
              NCoderPropID::kUsedMemorySize,
              NCoderPropID::kOrder
            };
            RINOK(encoder->SetCoderProperties(propIDs, props, sizeof(propIDs) / sizeof(propIDs[0])));
          }
          else
          {
          CMethodId methodId;
          switch(method)
          {
            case NFileHeader::NCompressionMethod::kBZip2:
              methodId = kMethodId_BZip2;
              _compressExtractVersion = NFileHeader::NCompressionMethod::kExtractVersion_BZip2;
              break;
            default:
              _compressExtractVersion = ((method == NFileHeader::NCompressionMethod::kDeflated64) ?
                  NFileHeader::NCompressionMethod::kExtractVersion_Deflate64 :
                  NFileHeader::NCompressionMethod::kExtractVersion_Deflate);
              methodId = kMethodId_ZipBase + method;
              break;
          }
          RINOK(CreateCoder(
              EXTERNAL_CODECS_LOC_VARS
              methodId, _compressEncoder, true));
          if (!_compressEncoder)
            return E_NOTIMPL;

          if (method == NFileHeader::NCompressionMethod::kDeflated ||
              method == NFileHeader::NCompressionMethod::kDeflated64)
          {
            NWindows::NCOM::CPropVariant props[] =
            {
              _options.Algo,
              _options.NumPasses,
              _options.NumFastBytes,
              _options.NumMatchFinderCycles
            };
            PROPID propIDs[] =
            {
              NCoderPropID::kAlgorithm,
              NCoderPropID::kNumPasses,
              NCoderPropID::kNumFastBytes,
              NCoderPropID::kMatchFinderCycles
            };
            int numProps = sizeof(propIDs) / sizeof(propIDs[0]);
            if (!_options.NumMatchFinderCyclesDefined)
              numProps--;
            CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
            _compressEncoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProperties);
            if (setCoderProperties)
            {
              RINOK(setCoderProperties->SetCoderProperties(propIDs, props, numProps));
            }
          }
          else if (method == NFileHeader::NCompressionMethod::kBZip2)
          {
            NWindows::NCOM::CPropVariant props[] =
            {
              _options.DicSize,
              _options.NumPasses
              #ifndef _7ZIP_ST
              , _options.NumThreads
              #endif
            };
            PROPID propIDs[] =
            {
              NCoderPropID::kDictionarySize,
              NCoderPropID::kNumPasses
              #ifndef _7ZIP_ST
              , NCoderPropID::kNumThreads
              #endif
            };
            CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
            _compressEncoder.QueryInterface(IID_ICompressSetCoderProperties, &setCoderProperties);
            if (setCoderProperties)
            {
              RINOK(setCoderProperties->SetCoderProperties(propIDs, props, sizeof(propIDs) / sizeof(propIDs[0])));
            }
          }
          }
        }
        CMyComPtr<ISequentialOutStream> outStreamNew;
        if (_options.PasswordIsDefined)
          outStreamNew = _cryptoStream;
        else
          outStreamNew = outStream;
        if (_compressExtractVersion > opRes.ExtractVersion)
          opRes.ExtractVersion = _compressExtractVersion;
        RINOK(_compressEncoder->Code(inCrcStream, outStreamNew, NULL, NULL, progress));
        break;
      }
    }

    RINOK(outStream->Seek(0, STREAM_SEEK_CUR, &opRes.PackSize));

    if (inCrcStreamSpec != 0)
    {
      opRes.CRC = inCrcStreamSpec->GetCRC();
      opRes.UnpackSize = inCrcStreamSpec->GetSize();
    }
    else
    {
      opRes.CRC = inSecCrcStreamSpec->GetCRC();
      opRes.UnpackSize = inSecCrcStreamSpec->GetSize();
    }

    if (_options.PasswordIsDefined)
    {
      if (opRes.PackSize < opRes.UnpackSize +
          (_options.IsAesMode ? _filterAesSpec->GetHeaderSize() : NCrypto::NZip::kHeaderSize))
        break;
    }
    else if (opRes.PackSize < opRes.UnpackSize)
      break;
  }
  if (_options.IsAesMode)
  {
    RINOK(_filterAesSpec->WriteFooter(outStream));
    RINOK(outStream->Seek(0, STREAM_SEEK_CUR, &opRes.PackSize));
  }
  opRes.Method = method;
  return S_OK;
}

}}
