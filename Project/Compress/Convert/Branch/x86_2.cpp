// x86_2.h

#include "StdAfx.h"
#include "x86_2.h"

#include "Windows/Defs.h"

inline UINT32 Swap4(UINT32 aValue)
{
  return (aValue << 24) | (aValue >> 24) | 
    ( (aValue >> 8) & 0xFF00) | ( (aValue << 8) & 0xFF0000);
}

inline bool IsJcc(BYTE aByte0, BYTE aByte1)
{
  return (aByte0 == 0x0F && (aByte1 & 0xF0) == 0x80);
}

#ifndef EXTRACT_ONLY

static bool inline Test86MSByte(BYTE aByte)
{
  return (aByte == 0 || aByte == 0xFF);
}

HRESULT CBCJ2_x86_Encoder::Flush()
{
  RETURN_IF_NOT_S_OK(m_MainStream.Flush());
  RETURN_IF_NOT_S_OK(m_E8Stream.Flush());
  RETURN_IF_NOT_S_OK(m_JumpStream.Flush());
  m_RangeEncoder.FlushData();
  return m_RangeEncoder.FlushStream();
}

const UINT32 kDefaultLimit = (1 << 24);

HRESULT CBCJ2_x86_Encoder::CodeReal(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes,
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams,
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress)
{
  if (aNumInStreams != 1 || aNumOutStreams != 4)
    return E_INVALIDARG;

  bool anSizeIsDefined = false;
  UINT64 anInSize;
  if (anInSizes != NULL)
    if (anInSizes[0] != NULL)
    {
      anInSize = *anInSizes[0];
      if (anInSize <= kDefaultLimit)
        anSizeIsDefined = true;
    }


  ISequentialInStream *anInStream = anInStreams[0];

  m_MainStream.Init(anOutStreams[0]);
  m_E8Stream.Init(anOutStreams[1]);
  m_JumpStream.Init(anOutStreams[2]);
  m_RangeEncoder.Init(anOutStreams[3]);
  for (int i = 0; i < 256; i++)
    m_StatusE8Encoder[i].Init();
  m_StatusE9Encoder.Init();
  m_StatusJccEncoder.Init();
  CCoderReleaser aReleaser(this);

  CComPtr<ICompressGetSubStreamSize> aGetSubStreamSize;
  {
    anInStream->QueryInterface(IID_ICompressGetSubStreamSize, (void **)&aGetSubStreamSize);
  }


  UINT32 aNowPos = 0;
  UINT64 aNowPos64 = 0;
  UINT32 aBufferPos = 0;
  UINT32 aProcessedSize;

  BYTE aPrevByte = 0;

  UINT64 aSubStreamIndex = 0;
  UINT64 aSubStreamStartPos  = 0;
  UINT64 aSubStreamEndPos = 0;

  while(true)
  {
    UINT32 aSize = kBufferSize - aBufferPos;
    RETURN_IF_NOT_S_OK(anInStream->Read(m_Buffer + aBufferPos, aSize, &aProcessedSize));
    UINT32 anEndPos = aBufferPos + aProcessedSize;
    
    if (anEndPos < 5)
    {
      // change it 
      for (aBufferPos = 0; aBufferPos < anEndPos; aBufferPos++)
      {
        BYTE aByte = m_Buffer[aBufferPos];
        m_MainStream.WriteByte(aByte);
        if (aByte == 0xE8)
          m_StatusE8Encoder[aPrevByte].Encode(&m_RangeEncoder, 0);
        else if (aByte == 0xE9)
          m_StatusE9Encoder.Encode(&m_RangeEncoder, 0);
        else if (IsJcc(aPrevByte, aByte))
          m_StatusJccEncoder.Encode(&m_RangeEncoder, 0);
        aPrevByte = aByte;
      }
      return Flush();
    }

    aBufferPos = 0;

    UINT32 aLimit = anEndPos - 5;
    while(aBufferPos <= aLimit)
    {
      BYTE aByte = m_Buffer[aBufferPos];
      m_MainStream.WriteByte(aByte);
      if (aByte != 0xE8 && aByte != 0xE9 && !IsJcc(aPrevByte, aByte))
      {
        aBufferPos++;
        aPrevByte = aByte;
        continue;
      }
      BYTE aNextByte = m_Buffer[aBufferPos + 4];
      UINT32 aSrc = 
        (UINT32(aNextByte) << 24) |
        (UINT32(m_Buffer[aBufferPos + 3]) << 16) |
        (UINT32(m_Buffer[aBufferPos + 2]) << 8) |
        (m_Buffer[aBufferPos + 1]);
      UINT32 aDest = (aNowPos + aBufferPos + 5) + aSrc;
      // if (Test86MSByte(aNextByte))
      bool aConvert;
      if (aGetSubStreamSize != NULL)
      {
        UINT64 aCurrentPos = (aNowPos64 + aBufferPos);
        while (aSubStreamEndPos < aCurrentPos)
        {
          UINT64 aSubStreamSize;
          HRESULT aResult = aGetSubStreamSize->GetSubStreamSize(aSubStreamIndex, &aSubStreamSize);
          if (aResult == S_OK)
          {
            aSubStreamStartPos = aSubStreamEndPos;
            aSubStreamEndPos += aSubStreamSize;          
            aSubStreamIndex++;
          }
          else if (aResult == S_FALSE || aResult == E_NOTIMPL)
          {
            aGetSubStreamSize.Release();
            aSubStreamStartPos = 0;
            aSubStreamEndPos = aSubStreamStartPos - 1;          
          }
          else
            return aResult;
        }
        if (aGetSubStreamSize == NULL)
        {
          if (anSizeIsDefined)
            aConvert = (aDest < anInSize);
          else
            aConvert = Test86MSByte(aNextByte);
        }
        else if (aSubStreamEndPos - aSubStreamStartPos > kDefaultLimit)
          aConvert = Test86MSByte(aNextByte);
        else
        {
          UINT64 aDest64 = (aCurrentPos + 5) + INT64(INT32(aSrc));
          aConvert = (aDest64 >= aSubStreamStartPos && aDest64 < aSubStreamEndPos);
        }
      }
      else if (anSizeIsDefined)
        aConvert = (aDest < anInSize);
      else
        aConvert = Test86MSByte(aNextByte);
      if (aConvert)
      {
        if (aByte == 0xE8)
          m_StatusE8Encoder[aPrevByte].Encode(&m_RangeEncoder, 1);
        else if (aByte == 0xE9)
          m_StatusE9Encoder.Encode(&m_RangeEncoder, 1);
        else 
          m_StatusJccEncoder.Encode(&m_RangeEncoder, 1);

        aDest = Swap4(aDest);

        aBufferPos += 5;
        if (aByte == 0xE8)
          m_E8Stream.WriteBytes(&aDest, sizeof(aDest));
        else 
          m_JumpStream.WriteBytes(&aDest, sizeof(aDest));
        aPrevByte = aNextByte;
      }
      else
      {
        if (aByte == 0xE8)
          m_StatusE8Encoder[aPrevByte].Encode(&m_RangeEncoder, 0);
        else if (aByte == 0xE9)
          m_StatusE9Encoder.Encode(&m_RangeEncoder, 0);
        else
          m_StatusJccEncoder.Encode(&m_RangeEncoder, 0);
        aBufferPos++;
        aPrevByte = aByte;
      }
    }
    aNowPos += aBufferPos;
    aNowPos64 += aBufferPos;

    if (aProgress != NULL)
    {
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&aNowPos64, NULL));
    }
 
    
    UINT32 i = 0;
    while(aBufferPos < anEndPos)
      m_Buffer[i++] = m_Buffer[aBufferPos++];
    aBufferPos = i;
  }
}

STDMETHODIMP CBCJ2_x86_Encoder::Code(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes,
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams,
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStreams, anInSizes, aNumInStreams,
      anOutStreams, anOutSizes,aNumOutStreams,
      aProgress);
  }
  catch(const NStream::COutByteWriteException &anOutWriteException)
  {
    return anOutWriteException.m_Result;
  }
  catch(...)
  {
    return S_FALSE;
  }
}

#endif

HRESULT CBCJ2_x86_Decoder::CodeReal(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes,
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams,
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress)
{
  if (aNumInStreams != 4 || aNumOutStreams != 1)
    return E_INVALIDARG;

  m_MainInStream.Init(anInStreams[0]);
  m_E8Stream.Init(anInStreams[1]);
  m_JumpStream.Init(anInStreams[2]);
  m_RangeDecoder.Init(anInStreams[3]);
  for (int i = 0; i < 256; i++)
    m_StatusE8Decoder[i].Init();
  m_StatusE9Decoder.Init();
  m_StatusJccDecoder.Init();

  m_OutStream.Init(anOutStreams[0]);

  CCoderReleaser aReleaser(this);

  BYTE aPrevByte = 0;
  UINT32 aProcessedBytes = 0;
  while(true)
  {
    if (aProcessedBytes > (1 << 20) && aProgress != NULL)
    {
      UINT64 aNowPos64 = m_OutStream.GetProcessedSize();
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(NULL, &aNowPos64));
      aProcessedBytes = 0;
    }
    aProcessedBytes++;
    BYTE aByte;
    if (!m_MainInStream.ReadByte(aByte))
      return Flush();
    m_OutStream.WriteByte(aByte);
    if (aByte != 0xE8 && aByte != 0xE9 && !IsJcc(aPrevByte, aByte))
    {
      aPrevByte = aByte;
      continue;
    }
    bool aStatus;
    if (aByte == 0xE8)
      aStatus = (m_StatusE8Decoder[aPrevByte].Decode(&m_RangeDecoder) == 1);
    else if (aByte == 0xE9)
      aStatus = (m_StatusE9Decoder.Decode(&m_RangeDecoder) == 1);
    else
      aStatus = (m_StatusJccDecoder.Decode(&m_RangeDecoder) == 1);
    if (aStatus)
    {
      UINT32 aSrc;
      if (aByte == 0xE8)
      {
        if (!m_E8Stream.ReadBytes(&aSrc, sizeof(aSrc)))
          return S_FALSE;
      }
      else
      {
        if (!m_JumpStream.ReadBytes(&aSrc, sizeof(aSrc)))
          return S_FALSE;
      }
      aSrc = Swap4(aSrc);
      UINT32 aDest = aSrc - (UINT32(m_OutStream.GetProcessedSize()) + 4) ;
      m_OutStream.WriteBytes(&aDest, sizeof(aDest));
      aPrevByte = (aDest >> 24);
      aProcessedBytes += 4;
    }
    else
      aPrevByte = aByte;
  }
}

STDMETHODIMP CBCJ2_x86_Decoder::Code(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes,
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams,
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStreams, anInSizes, aNumInStreams,
      anOutStreams, anOutSizes,aNumOutStreams,
      aProgress);
  }
  catch(const NStream::COutByteWriteException &anOutWriteException)
  {
    return anOutWriteException.m_Result;
  }
  catch(...)
  {
    return S_FALSE;
  }
}
