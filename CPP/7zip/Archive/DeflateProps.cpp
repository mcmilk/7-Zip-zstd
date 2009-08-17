// DeflateProps.cpp

#include "StdAfx.h"

#include "Windows/PropVariant.h"

#include "Common/ParseProperties.h"

#include "DeflateProps.h"

namespace NArchive {

static const UInt32 kAlgo1 = 0;
static const UInt32 kAlgo5 = 1;

static const UInt32 kPasses1 = 1;
static const UInt32 kPasses7 = 3;
static const UInt32 kPasses9 = 10;

static const UInt32 kFb1 = 32;
static const UInt32 kFb7 = 64;
static const UInt32 kFb9 = 128;

void CDeflateProps::Normalize()
{
  UInt32 level = Level;
  if (level == 0xFFFFFFFF)
    level = 5;
  
  if (Algo == 0xFFFFFFFF)
    Algo = (level >= 5 ?
      kAlgo5 :
      kAlgo1);

  if (NumPasses == 0xFFFFFFFF)
    NumPasses =
      (level >= 9 ? kPasses9 :
      (level >= 7 ? kPasses7 :
                    kPasses1));
  if (Fb == 0xFFFFFFFF)
    Fb =
      (level >= 9 ? kFb9 :
      (level >= 7 ? kFb7 :
                    kFb1));
}

HRESULT CDeflateProps::SetCoderProperties(ICompressSetCoderProperties *setCoderProperties)
{
  Normalize();

  NWindows::NCOM::CPropVariant props[] =
  {
    Algo,
    NumPasses,
    Fb,
    Mc
  };
  PROPID propIDs[] =
  {
    NCoderPropID::kAlgorithm,
    NCoderPropID::kNumPasses,
    NCoderPropID::kNumFastBytes,
    NCoderPropID::kMatchFinderCycles
  };
  int numProps = sizeof(propIDs) / sizeof(propIDs[0]);
  if (!McDefined)
    numProps--;
  return setCoderProperties->SetCoderProperties(propIDs, props, numProps);
}

HRESULT CDeflateProps::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProps)
{
  Init();
  for (int i = 0; i < numProps; i++)
  {
    UString name = names[i];
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;
    const PROPVARIANT &prop = values[i];
    if (name[0] == L'X')
    {
      UInt32 a = 9;
      RINOK(ParsePropValue(name.Mid(1), prop, a));
      Level = a;
    }
    else if (name.Left(1) == L"A")
    {
      UInt32 a = kAlgo5;
      RINOK(ParsePropValue(name.Mid(1), prop, a));
      Algo = a;
    }
    else if (name.Left(4) == L"PASS")
    {
      UInt32 a = kPasses9;
      RINOK(ParsePropValue(name.Mid(4), prop, a));
      NumPasses = a;
    }
    else if (name.Left(2) == L"FB")
    {
      UInt32 a = kFb9;
      RINOK(ParsePropValue(name.Mid(2), prop, a));
      Fb = a;
    }
    else if (name.Left(2) == L"MC")
    {
      UInt32 a = 0xFFFFFFFF;
      RINOK(ParsePropValue(name.Mid(2), prop, a));
      Mc = a;
      McDefined = true;
    }
    else
      return E_INVALIDARG;
  }
  return S_OK;
}

}
