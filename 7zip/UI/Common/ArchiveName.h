// ArchiveName.h

#pragma once

#ifndef __ARCHIVENAME_H
#define __ARCHIVENAME_H

#include "Common/String.h"

CSysString CreateArchiveName(const CSysString &srcName, bool fromPrev, bool keepName);

#endif