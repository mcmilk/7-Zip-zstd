// WorkDir.h

#pragma once

#ifndef __WORKDIR_H
#define __WORKDIR_H

#include "../Common/ZipRegistry.h"

UString GetWorkDir(const NWorkDir::CInfo &workDirInfo, 
    const UString &archiveName);

#endif

