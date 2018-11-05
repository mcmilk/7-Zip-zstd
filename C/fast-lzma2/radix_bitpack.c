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

#define RMF_BITPACK

#define RADIX_MAX_LENGTH BITPACK_MAX_LENGTH

#define InitMatchLink(index, link) tbl->table[index] = link

#define GetMatchLink(link) (tbl->table[link] & RADIX_LINK_MASK)

#define GetInitialMatchLink(index) tbl->table[index]

#define GetMatchLength(index) (tbl->table[index] >> RADIX_LINK_BITS)

#define SetMatchLink(index, link, length) tbl->table[index] = (link) | ((U32)(length) << RADIX_LINK_BITS)

#define SetMatchLength(index, link, length) tbl->table[index] = (link) | ((U32)(length) << RADIX_LINK_BITS)

#define SetMatchLinkAndLength(index, link, length) tbl->table[index] = (link) | ((U32)(length) << RADIX_LINK_BITS)

#define SetNull(index) tbl->table[index] = RADIX_NULL_LINK

#define IsNull(index) (tbl->table[index] == RADIX_NULL_LINK)

BYTE* RMF_bitpackAsOutputBuffer(FL2_matchTable* const tbl, size_t const index)
{
    return (BYTE*)(tbl->table + index);
}

/* Restrict the match lengths so that they don't reach beyond index */
void RMF_bitpackLimitLengths(FL2_matchTable* const tbl, size_t const index)
{
    DEBUGLOG(5, "RMF_limitLengths : end %u, max length %u", (U32)index, RADIX_MAX_LENGTH);
    SetNull(index - 1);
    for (U32 length = 2; length < RADIX_MAX_LENGTH && length <= index; ++length) {
        U32 const link = tbl->table[index - length];
        if (link != RADIX_NULL_LINK) {
            tbl->table[index - length] = (MIN(length, link >> RADIX_LINK_BITS) << RADIX_LINK_BITS) | (link & RADIX_LINK_MASK);
        }
    }
}

#include "radix_engine.h"