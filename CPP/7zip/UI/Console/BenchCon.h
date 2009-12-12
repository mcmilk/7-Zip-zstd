// BenchCon.h

#ifndef __BENCH_CON_H
#define __BENCH_CON_H

#include <stdio.h>

#include "../../Common/CreateCoder.h"

HRESULT LzmaBenchCon(
    DECL_EXTERNAL_CODECS_LOC_VARS
    FILE *f, UInt32 numIterations, UInt32 numThreads, UInt32 dictionary);

HRESULT CrcBenchCon(FILE *f, UInt32 numIterations, UInt32 numThreads, UInt32 dictionary);

#endif
