// RpmHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/MyString.h"

#include "Windows/PropVariant.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

using namespace NWindows;

#define Get16(p) GetBe16(p)
#define Get32(p) GetBe32(p)

namespace NArchive {
namespace NRpm {

/* Reference: lib/signature.h of rpm package */
#define RPMSIG_NONE         0  /* Do not change! */
/* The following types are no longer generated */
#define RPMSIG_PGP262_1024  1  /* No longer generated */ /* 256 byte */
/* These are the new-style signatures.  They are Header structures.    */
/* Inside them we can put any number of any type of signature we like. */

#define RPMSIG_HEADERSIG    5  /* New Header style signature */

const UInt32 kLeadSize = 96;
struct CLead
{
  unsigned char Magic[4];
  unsigned char Major;  // not supported  ver1, only support 2,3 and lator
  unsigned char Minor;
  UInt16 Type;
  UInt16 ArchNum;
  char Name[66];
  UInt16 OSNum;
  UInt16 SignatureType;
  char Reserved[16];  // pad to 96 bytes -- 8 byte aligned
  bool MagicCheck() const
    { return Magic[0] == 0xed && Magic[1] == 0xab && Magic[2] == 0xee && Magic[3] == 0xdb; };
};
  
const UInt32 kEntryInfoSize = 16;
/*
struct CEntryInfo
{
  int Tag;
  int Type;
  int Offset; // Offset from beginning of data segment, only defined on disk
  int Count;
};
*/

// case: SignatureType == RPMSIG_HEADERSIG
const UInt32 kCSigHeaderSigSize = 16;
struct CSigHeaderSig
{
  unsigned char Magic[4];
  UInt32 Reserved;
  UInt32 IndexLen;  // count of index entries
  UInt32 DataLen;   // number of bytes
  bool MagicCheck()
    { return Magic[0] == 0x8e && Magic[1] == 0xad && Magic[2] == 0xe8 && Magic[3] == 0x01; };
  UInt32 GetLostHeaderLen()
    { return IndexLen * kEntryInfoSize + DataLen; };
};

static HRESULT RedSigHeaderSig(IInStream *inStream, CSigHeaderSig &h)
{
  char dat[kCSigHeaderSigSize];
  char *cur = dat;
  RINOK(ReadStream_FALSE(inStream, dat, kCSigHeaderSigSize));
  memcpy(h.Magic, cur, 4);
  cur += 4;
  cur += 4;
  h.IndexLen = Get32(cur);
  cur += 4;
  h.DataLen = Get32(cur);
  return S_OK;
}

HRESULT OpenArchive(IInStream *inStream)
{
  UInt64 pos;
  char leadData[kLeadSize];
  char *cur = leadData;
  CLead lead;
  RINOK(ReadStream_FALSE(inStream, leadData, kLeadSize));
  memcpy(lead.Magic, cur, 4);
  cur += 4;
  lead.Major = *cur++;
  lead.Minor = *cur++;
  lead.Type = Get16(cur);
  cur += 2;
  lead.ArchNum = Get16(cur);
  cur += 2;
  memcpy(lead.Name, cur, sizeof(lead.Name));
  cur += sizeof(lead.Name);
  lead.OSNum = Get16(cur);
  cur += 2;
  lead.SignatureType = Get16(cur);
  cur += 2;

  if (!lead.MagicCheck() || lead.Major < 3)
    return S_FALSE;

  CSigHeaderSig sigHeader, header;
  if (lead.SignatureType == RPMSIG_NONE)
  {
    ;
  }
  else if (lead.SignatureType == RPMSIG_PGP262_1024)
  {
    UInt64 pos;
    RINOK(inStream->Seek(256, STREAM_SEEK_CUR, &pos));
  }
  else if (lead.SignatureType == RPMSIG_HEADERSIG)
  {
    RINOK(RedSigHeaderSig(inStream, sigHeader));
    if (!sigHeader.MagicCheck())
      return S_FALSE;
    UInt32 len = sigHeader.GetLostHeaderLen();
    RINOK(inStream->Seek(len, STREAM_SEEK_CUR, &pos));
    if ((pos % 8) != 0)
    {
      RINOK(inStream->Seek((pos / 8 + 1) * 8 - pos,
          STREAM_SEEK_CUR, &pos));
    }
  }
  else
    return S_FALSE;

  RINOK(RedSigHeaderSig(inStream, header));
  if (!header.MagicCheck())
    return S_FALSE;
  int headerLen = header.GetLostHeaderLen();
  if (headerLen == -1)
    return S_FALSE;
  RINOK(inStream->Seek(headerLen, STREAM_SEEK_CUR, &pos));
  return S_OK;
}

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  UInt64 _pos;
  UInt64 _size;
  Byte _sig[4];
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

STATPROPSTG kProps[] =
{
  { NULL, kpidSize, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  switch(propID) { case kpidMainSubfile: prop = (UInt32)0; break; }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  try
  {
    Close();
    if (OpenArchive(inStream) != S_OK)
      return S_FALSE;
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &_pos));
    RINOK(ReadStream_FALSE(inStream, _sig, sizeof(_sig) / sizeof(_sig[0])));
    UInt64 endPosition;
    RINOK(inStream->Seek(0, STREAM_SEEK_END, &endPosition));
    _size = endPosition - _pos;
    _stream = inStream;
    return S_OK;
  }
  catch(...) { return S_FALSE; }
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidSize:
    case kpidPackSize:
      prop = _size;
      break;
    case kpidExtension:
    {
      char s[32];
      MyStringCopy(s, "cpio.");
      const char *ext;
      if (_sig[0] == 0x1F && _sig[1] == 0x8B)
        ext = "gz";
      else if (_sig[0] == 'B' && _sig[1] == 'Z' && _sig[2] == 'h')
        ext = "bz2";
      else
        ext = "lzma";
      MyStringCopy(s + MyStringLen(s), ext);
      prop = s;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  RINOK(extractCallback->SetTotal(_size));
  CMyComPtr<ISequentialOutStream> outStream;
  Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &outStream, askMode));
  if (!testMode && !outStream)
    return S_OK;
  RINOK(extractCallback->PrepareOperation(askMode));

  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);
  
  RINOK(_stream->Seek(_pos, STREAM_SEEK_SET, NULL));
  RINOK(copyCoder->Code(_stream, outStream, NULL, NULL, progress));
  outStream.Release();
  return extractCallback->SetOperationResult(NExtract::NOperationResult::kOK);
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 /* index */, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  return CreateLimitedInStream(_stream, _pos, _size, stream);
  COM_TRY_END
}

static IInArchive *CreateArc() { return new NArchive::NRpm::CHandler; }

static CArcInfo g_ArcInfo =
  { L"Rpm", L"rpm", 0, 0xEB, { 0xED, 0xAB, 0xEE, 0xDB}, 4, false, CreateArc, 0 };

REGISTER_ARC(Rpm)

}}
