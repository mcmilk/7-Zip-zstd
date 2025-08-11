/* 
 * Because of code deduplication we'll use zstd/bitstream.h here
 */
#include "../zstd/zstd_deps.h"   /* ZSTD_memset */
#include "../zstd/bitstream.h"

#define BIT_highbit32 ZSTD_highbit32