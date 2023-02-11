#!/bin/sh
# /TR 2020-05-27

for i in *.c *.h; do
  sed -i 's|include "../common/|include "|g' "$i"
  sed -i 's|include "../legacy/|include "|g' "$i"
  sed -i 's|include "../zstd.h|include "zstd.h|g' "$i"
  sed -i 's|include "../zstd_errors.h|include "zstd_errors.h|g' "$i"
done

exit

# then put these to "zstd.h"

/* disable some warnings /TR */
#if defined(_MSC_VER)
#pragma warning(disable : 4090)
#endif
