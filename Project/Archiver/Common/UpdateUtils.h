// UpdateUtils.h

#pragma once

#ifndef __UPDATEUTILS_H
#define __UPDATEUTILS_H

#include "../Common/ZipSettings.h"

CSysString GetWorkDir(const NZipSettings::NWorkDir::CInfo &aWorkDirInfo,
    const CSysString &anArchiveName);

#endif

