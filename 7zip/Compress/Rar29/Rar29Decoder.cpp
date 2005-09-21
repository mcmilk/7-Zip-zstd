// Rar29Decoder.cpp
// According to unRAR license,
// this code may not be used to develop a 
// RAR (WinRAR) compatible archiver
 
#include "StdAfx.h"

#include "Rar29Decoder.h"

#include "Windows/Defs.h"

// #include "Original/unpack15.cpp"

ErrorHandler ErrHandler;

namespace NCompress {
namespace NRar29 {

CDecoder::CDecoder():
  m_IsSolid(false),
  // DataIO(NULL)
  DataIO()
{
  Unp = new Unpack(&DataIO);
  Unp->Init(NULL);
}

CDecoder::~CDecoder()
{
  delete Unp;
}

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  // CCoderReleaser coderReleaser(this);

  DataIO.CurUnpRead=0;
  DataIO.CurUnpWrite=0;
  DataIO.UnpFileCRC=0xffffffff;
  DataIO.PackedCRC=0xffffffff;
  /*
  DataIO.SetEncryption(
        0,0,
        NULL,false);
  */

  DataIO.SetPackedSizeToRead(*inSize);
  DataIO.SetFiles(inStream, outStream, progress);
  DataIO.SetTestMode(false);
  // DataIO.SetSkipUnpCRC(SkipSolid);
  /*
  if (!TestMode && Arc.NewLhd.FullUnpSize>0x1000)
        CurFile.Prealloc(Arc.NewLhd.FullUnpSize);

      CurFile.SetAllowDelete(!Cmd->KeepBroken);
  */
  
  Unp->SetDestSize(*outSize);

  Unp->DoUnpack(29, m_IsSolid);
  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CExitCode &exitCode) { return exitCode.Result; }
  catch(...) { return S_FALSE; }
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size)
{
  if (size < 1)
    return E_INVALIDARG;
  m_IsSolid = (data[0] != 0);
  return S_OK;
}

}


namespace NRar15 {

CDecoder::CDecoder():
  m_IsSolid(false),
  DataIO()
{
  Unp=new Unpack(&DataIO);
  Unp->Init(NULL);
}

CDecoder::~CDecoder()
{
  delete Unp;
}


STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  // CCoderReleaser coderReleaser(this);

  DataIO.CurUnpRead=0;
  DataIO.CurUnpWrite=0;
  DataIO.UnpFileCRC=0xffffffff;
  DataIO.PackedCRC=0xffffffff;
  /*
  DataIO.SetEncryption(
        0,0,
        NULL,false);
  */

  DataIO.SetPackedSizeToRead(*inSize);
  DataIO.SetFiles(inStream, outStream, progress);
  DataIO.SetTestMode(false);
  // DataIO.SetSkipUnpCRC(SkipSolid);
  /*
  if (!TestMode && Arc.NewLhd.FullUnpSize>0x1000)
        CurFile.Prealloc(Arc.NewLhd.FullUnpSize);

      CurFile.SetAllowDelete(!Cmd->KeepBroken);
  */
  Unp->SetDestSize(*outSize);
  Unp->DoUnpack(15, m_IsSolid);
  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, 
    const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CExitCode &exitCode) { return exitCode.Result; }
  catch(...) { return S_FALSE;}
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size)
{
  if (size < 1)
    return E_INVALIDARG;
  m_IsSolid = (data[0] != 0);
  return S_OK;
}

}

}
