// ZipRegistry.h

#pragma once

#ifndef __ZIPREGISTRY_H
#define __ZIPREGISTRY_H

#include "Windows/Registry.h"
#include "ZipSettings.h"

class CZipRegistryManager
{
  NWindows::NRegistry::CKey m_CUKey;
  // NWindows::NRegistry::CKey m_LMKey;
public:
  CZipRegistryManager();
  void SaveListViewInfo(const CLSID &aClassID, const NZipSettings::CListViewInfo &aViewInfo);
  void ReadListViewInfo(const CLSID &aClassID, NZipSettings::CListViewInfo &aViewInfo);

  void SaveExtractionInfo(const NZipSettings::NExtraction::CInfo &anInfo);
  void ReadExtractionInfo(NZipSettings::NExtraction::CInfo &anInfo);

  void SaveCompressionInfo(const NZipSettings::NCompression::CInfo &anInfo);
  void ReadCompressionInfo(NZipSettings::NCompression::CInfo &anInfo);

  void SaveWorkDirInfo(const NZipSettings::NWorkDir::CInfo &anInfo);
  void ReadWorkDirInfo(NZipSettings::NWorkDir::CInfo &anInfo);
  
};

#endif