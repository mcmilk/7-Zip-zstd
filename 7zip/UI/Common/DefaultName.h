// DefaultName.h

#pragma once

#ifndef __DEFAULTNAME_H
#define __DEFAULTNAME_H

#include "Common/String.h"

UString GetDefaultName(const UString &fullFileName, 
    const UString &extension, const UString &addSubExtension);

#endif