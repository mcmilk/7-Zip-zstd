// WorkDir.h

#pragma once

#ifndef __WORKDIR_H
#define __WORKDIR_H

#include "../Common/ZipRegistry.h"

CSysString GetWorkDir(const NWorkDir::CInfo &workDirInfo,
    const CSysString &archiveName);

#endif

