// FileStreams.cpp

#include "StdAfx.h"
#include "FileStreams.h"

static inline HRESULT ConvertBoolToHRESULT(bool result)
{
  // return result ? S_OK: E_FAIL;
  return result ? S_OK: (::GetLastError());
}

bool CInFileStream::Open(LPCTSTR fileName)
{
  return File.Open(fileName);
}

#ifndef _UNICODE
bool CInFileStream::Open(LPCWSTR fileName)
{
  return File.Open(fileName);
}
#endif

STDMETHODIMP CInFileStream::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  bool result = File.Read(data, size, realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return ConvertBoolToHRESULT(result);
}
  
STDMETHODIMP CInFileStream::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  return Read(data, size, processedSize);
}


STDMETHODIMP CInFileStream::Seek(INT64 offset, UINT32 seekOrigin, 
    UINT64 *newPosition)
{
  if(seekOrigin >= 3)
    return STG_E_INVALIDFUNCTION;
  UINT64 realNewPosition;
  bool result = File.Seek(offset, seekOrigin, realNewPosition);
  if(newPosition != NULL)
    *newPosition = realNewPosition;
  return ConvertBoolToHRESULT(result);
}

STDMETHODIMP CInFileStream::GetSize(UINT64 *size)
{
  return ConvertBoolToHRESULT(File.GetLength(*size));
}


//////////////////////////
// COutFileStream

bool COutFileStream::Open(LPCTSTR fileName)
{
  File.SetOpenCreationDispositionCreateAlways();
  return File.Open(fileName);
}

#ifndef _UNICODE
bool COutFileStream::Open(LPCWSTR fileName)
{
  File.SetOpenCreationDispositionCreateAlways();
  return File.Open(fileName);
}
#endif

STDMETHODIMP COutFileStream::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  bool result = File.Write(data, size, realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return ConvertBoolToHRESULT(result);
}
  
STDMETHODIMP COutFileStream::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
  return Write(data, size, processedSize);
}


STDMETHODIMP COutFileStream::Seek(INT64 offset, UINT32 seekOrigin, 
    UINT64 *newPosition)
{
  if(seekOrigin >= 3)
    return STG_E_INVALIDFUNCTION;
  UINT64 realNewPosition;
  bool result = File.Seek(offset, seekOrigin, realNewPosition);
  if(newPosition != NULL)
    *newPosition = realNewPosition;
  return ConvertBoolToHRESULT(result);
}

STDMETHODIMP COutFileStream::SetSize(INT64 newSize)
{
  UINT64 currentPos;
  if(!File.Seek(0, FILE_CURRENT, currentPos))
    return E_FAIL;
  bool result = File.SetLength(newSize);
  UINT64 currentPos2;
  result = result && File.Seek(currentPos, currentPos2);
  return result ? S_OK : E_FAIL;
}
