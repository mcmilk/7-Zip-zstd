// FileStreams.cpp

#include "StdAfx.h"
#include "FileStreams.h"

static inline HRESULT ConvertBoolToHRESULT(bool aResult)
{
  // return aResult ? S_OK: E_FAIL;
  return aResult ? S_OK: (::GetLastError());
}

bool CInFileStream::Open(LPCTSTR aFileName)
{
  return m_File.Open(aFileName);
}

STDMETHODIMP CInFileStream::Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  bool aResult = m_File.Read(aData, aSize, aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return ConvertBoolToHRESULT(aResult);
}
  
STDMETHODIMP CInFileStream::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Read(aData, aSize, aProcessedSize);
}


STDMETHODIMP CInFileStream::Seek(INT64 anOffset, UINT32 aSeekOrigin, 
    UINT64 *aNewPosition)
{
  if(aSeekOrigin >= 3)
    return STG_E_INVALIDFUNCTION;
  UINT64 aNewPositionReal;
  bool aResult = m_File.Seek(anOffset, aSeekOrigin, aNewPositionReal);
  if(aNewPosition != NULL)
    *aNewPosition = aNewPositionReal;
  return ConvertBoolToHRESULT(aResult);
}


//////////////////////////
// COutFileStream


bool COutFileStream::Open(LPCTSTR aFileName)
{
  m_File.SetOpenCreationDispositionCreateAlways();
  return m_File.Open(aFileName);
}

STDMETHODIMP COutFileStream::Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  bool aResult = m_File.Write(aData, aSize, aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return ConvertBoolToHRESULT(aResult);
}
  
STDMETHODIMP COutFileStream::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Write(aData, aSize, aProcessedSize);
}


STDMETHODIMP COutFileStream::Seek(INT64 anOffset, UINT32 aSeekOrigin, 
    UINT64 *aNewPosition)
{
  if(aSeekOrigin >= 3)
    return STG_E_INVALIDFUNCTION;
  UINT64 aNewPositionReal;
  bool aResult = m_File.Seek(anOffset, aSeekOrigin, aNewPositionReal);
  if(aNewPosition != NULL)
    *aNewPosition = aNewPositionReal;
  return ConvertBoolToHRESULT(aResult);
}

STDMETHODIMP COutFileStream::SetSize(INT64 aNewSize)
{
  UINT64 aCurrentPos;
  if(!m_File.Seek(0, FILE_CURRENT, aCurrentPos))
    return E_FAIL;
  bool aResult = m_File.SetLength(aNewSize);
  UINT64 aCurrentPos2;
  aResult = aResult && m_File.Seek(aCurrentPos, aCurrentPos2);
  return aResult ? S_OK : E_FAIL;
}
