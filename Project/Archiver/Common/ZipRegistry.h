// ZipRegistry.h

#pragma once

#ifndef __ZIPREGISTRY_H
#define __ZIPREGISTRY_H

#include "Windows/Registry.h"
#include "ZipSettings.h"

namespace NZipRegistryManager
{
  void SaveExtractionInfo(const NZipSettings::NExtraction::CInfo &info);
  void ReadExtractionInfo(NZipSettings::NExtraction::CInfo &info);

  void SaveCompressionInfo(const NZipSettings::NCompression::CInfo &info);
  void ReadCompressionInfo(NZipSettings::NCompression::CInfo &info);

  void SaveWorkDirInfo(const NZipSettings::NWorkDir::CInfo &info);
  void ReadWorkDirInfo(NZipSettings::NWorkDir::CInfo &info);
  
};

#endif