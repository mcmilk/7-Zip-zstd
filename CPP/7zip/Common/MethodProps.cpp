// MethodProps.cpp

#include "StdAfx.h"

#include "MethodProps.h"
#include "../../Common/MyCom.h"

static UInt64 k_LZMA = 0x030101;
// static UInt64 k_LZMA2 = 0x030102;

HRESULT SetMethodProperties(const CMethod &method, const UInt64 *inSizeForReduce, IUnknown *coder)
{
  bool tryReduce = false;
  UInt32 reducedDictionarySize = 1 << 10;
  if (inSizeForReduce != 0 && (method.Id == k_LZMA /* || methodFull.MethodID == k_LZMA2 */))
  {
    for (;;)
    {
      const UInt32 step = (reducedDictionarySize >> 1);
      if (reducedDictionarySize >= *inSizeForReduce)
      {
        tryReduce = true;
        break;
      }
      reducedDictionarySize += step;
      if (reducedDictionarySize >= *inSizeForReduce)
      {
        tryReduce = true;
        break;
      }
      if (reducedDictionarySize >= ((UInt32)3 << 30))
        break;
      reducedDictionarySize += step;
    }
  }

  {
    int numProperties = method.Properties.Size();
    CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
    coder->QueryInterface(IID_ICompressSetCoderProperties, (void **)&setCoderProperties);
    if (setCoderProperties == NULL)
    {
      if (numProperties != 0)
        return E_INVALIDARG;
    }
    else
    {
      CRecordVector<PROPID> propIDs;
      NWindows::NCOM::CPropVariant *values = new NWindows::NCOM::CPropVariant[numProperties];
      HRESULT res = S_OK;
      try
      {
        for (int i = 0; i < numProperties; i++)
        {
          const CProp &prop = method.Properties[i];
          propIDs.Add(prop.Id);
          NWindows::NCOM::CPropVariant &value = values[i];
          value = prop.Value;
          // if (tryReduce && prop.Id == NCoderPropID::kDictionarySize && value.vt == VT_UI4 && reducedDictionarySize < value.ulVal)
          if (tryReduce)
            if (prop.Id == NCoderPropID::kDictionarySize)
              if (value.vt == VT_UI4)
                if (reducedDictionarySize < value.ulVal)
            value.ulVal = reducedDictionarySize;
        }
        CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
        coder->QueryInterface(IID_ICompressSetCoderProperties, (void **)&setCoderProperties);
        res = setCoderProperties->SetCoderProperties(&propIDs.Front(), values, numProperties);
      }
      catch(...)
      {
        delete []values;
        throw;
      }
      delete []values;
      RINOK(res);
    }
  }
 
  /*
  CMyComPtr<ICompressWriteCoderProperties> writeCoderProperties;
  coder->QueryInterface(IID_ICompressWriteCoderProperties, (void **)&writeCoderProperties);
  if (writeCoderProperties != NULL)
  {
    CSequentialOutStreamImp *outStreamSpec = new CSequentialOutStreamImp;
    CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
    outStreamSpec->Init();
    RINOK(writeCoderProperties->WriteCoderProperties(outStream));
    size_t size = outStreamSpec->GetSize();
    filterProps.SetCapacity(size);
    memmove(filterProps, outStreamSpec->GetBuffer(), size);
  }
  */
  return S_OK;
}

