// CoderLoader.cpp

#include "StdAfx.h"

#include "CoderLoader.h"
#include "FilterCoder.h"

HRESULT CCoderLibrary::CreateCoderSpec(REFGUID clsID, ICompressCoder **coder)
{
  HRESULT result = CreateObject(clsID, IID_ICompressCoder, (void **)coder);
  if (result == S_OK || result != E_NOINTERFACE)
    return result;
  CMyComPtr<ICompressFilter> filter;
  RINOK(CreateObject(clsID, IID_ICompressFilter, (void **)&filter));
  CFilterCoder *filterCoderSpec = new CFilterCoder;
  CMyComPtr<ICompressCoder> filterCoder = filterCoderSpec;
  filterCoderSpec->Filter = filter;
  *coder = filterCoder.Detach();
  return S_OK;
}


HRESULT CCoderLibrary::LoadAndCreateCoderSpec(LPCTSTR filePath, REFGUID clsID, ICompressCoder **coder)
{
  CCoderLibrary libTemp;
  if (!libTemp.Load(filePath))
    return GetLastError();
  RINOK(libTemp.CreateCoderSpec(clsID, coder));
  Attach(libTemp.Detach());
  return S_OK;
}
