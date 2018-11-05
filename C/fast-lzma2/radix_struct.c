/*
* Copyright (c) 2018, Conor McCarthy
* All rights reserved.
*
* This source code is licensed under both the BSD-style license (found in the
* LICENSE file in the root directory of this source tree) and the GPLv2 (found
* in the COPYING file in the root directory of this source tree).
* You may select, at your option, one of the above-listed licenses.
*/

#include "mem.h"          /* U32, U64 */
#include "fl2threading.h"
#include "fl2_internal.h"
#include "radix_internal.h"

typedef struct FL2_matchTable_s FL2_matchTable;

#undef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define RMF_STRUCTURED

#define RADIX_MAX_LENGTH STRUCTURED_MAX_LENGTH

#define InitMatchLink(index, link) ((RMF_unit*)tbl->table)[(index) >> UNIT_BITS].links[(index) & UNIT_MASK] = (U32)(link)

#define GetMatchLink(index) ((RMF_unit*)tbl->table)[(index) >> UNIT_BITS].links[(index) & UNIT_MASK]

#define GetInitialMatchLink(index) ((RMF_unit*)tbl->table)[(index) >> UNIT_BITS].links[(index) & UNIT_MASK]

#define GetMatchLength(index) ((RMF_unit*)tbl->table)[(index) >> UNIT_BITS].lengths[(index) & UNIT_MASK]

#define SetMatchLink(index, link, length) ((RMF_unit*)tbl->table)[(index) >> UNIT_BITS].links[(index) & UNIT_MASK] = (U32)(link)

#define SetMatchLength(index, link, length) ((RMF_unit*)tbl->table)[(index) >> UNIT_BITS].lengths[(index) & UNIT_MASK] = (BYTE)(length)

#define SetMatchLinkAndLength(index, link, length) { size_t i_ = (index) >> UNIT_BITS, u_ = (index) & UNIT_MASK; ((RMF_unit*)tbl->table)[i_].links[u_] = (U32)(link); ((RMF_unit*)tbl->table)[i_].lengths[u_] = (BYTE)(length); }

#define SetNull(index) ((RMF_unit*)tbl->table)[(index) >> UNIT_BITS].links[(index) & UNIT_MASK] = RADIX_NULL_LINK

#define IsNull(index) (((RMF_unit*)tbl->table)[(index) >> UNIT_BITS].links[(index) & UNIT_MASK] == RADIX_NULL_LINK)

BYTE* RMF_structuredAsOutputBuffer(FL2_matchTable* const tbl, size_t const index)
{
    return (BYTE*)((RMF_unit*)tbl->table + (index >> UNIT_BITS) + ((index & UNIT_MASK) != 0));
}

/* Restrict the match lengths so that they don't reach beyond index */
void RMF_structuredLimitLengths(FL2_matchTable* const tbl, size_t const index)
{
    DEBUGLOG(5, "RMF_limitLengths : end %u, max length %u", (U32)index, RADIX_MAX_LENGTH);
    SetNull(index - 1);
    for (size_t length = 2; length < RADIX_MAX_LENGTH && length <= index; ++length) {
        size_t const i = (index - length) >> UNIT_BITS;
        size_t const u = (index - length) & UNIT_MASK;
        if (((RMF_unit*)tbl->table)[i].links[u] != RADIX_NULL_LINK) {
            ((RMF_unit*)tbl->table)[i].lengths[u] = MIN((BYTE)length, ((RMF_unit*)tbl->table)[i].lengths[u]);
        }
    }
}

#include "radix_engine.h"