/*
* Copyright (c) 2018, Conor McCarthy
* All rights reserved.
*
* This source code is licensed under both the BSD-style license (found in the
* LICENSE file in the root directory of this source tree) and the GPLv2 (found
* in the COPYING file in the root directory of this source tree).
* You may select, at your option, one of the above-listed licenses.
*/

#include <stdio.h>  
#include "count.h"

#define MAX_READ_BEYOND_DEPTH 2

/* If a repeating byte is found, fill that section of the table with matches of distance 1 */
static size_t HandleRepeat(FL2_matchTable* const tbl, const BYTE* const data_block, size_t const start, ptrdiff_t const block_size, ptrdiff_t i, size_t const radix_16)
{
    ptrdiff_t const rpt_index = i - (MAX_REPEAT / 2 - 2);
    ptrdiff_t rpt_end;
    /* Set the head to the first byte of the repeat and adjust the count */
    tbl->list_heads[radix_16].head = (U32)(rpt_index - 1);
    tbl->list_heads[radix_16].count -= MAX_REPEAT / 2 - 2;
    /* Find the end */
    i += ZSTD_count(data_block + i + 2, data_block + i + 1, data_block + block_size);
    rpt_end = i;
    /* No point if it's in the overlap region */
    if (i >= (ptrdiff_t)start) {
        U32 len = 2;
        /* Set matches at distance 1 and available length */
        for (; i >= rpt_index && len <= RADIX_MAX_LENGTH; --i) {
            SetMatchLinkAndLength(i, (U32)(i - 1), len);
            ++len;
        }
        /* Set matches at distance 1 and max length */
        for (; i >= rpt_index; --i) {
            SetMatchLinkAndLength(i, (U32)(i - 1), RADIX_MAX_LENGTH);
        }
    }
    return rpt_end;
}

/* If a 2-byte repeat is found, fill that section of the table with matches of distance 2 */
static size_t HandleRepeat2(FL2_matchTable* const tbl, const BYTE* const data_block, size_t const start, ptrdiff_t const block_size, ptrdiff_t i, size_t const radix_16)
{
    size_t radix_16_rev;
    ptrdiff_t const rpt_index = i - (MAX_REPEAT - 3);
    ptrdiff_t rpt_end;

    /* Set the head to the first byte of the repeat and adjust the count */
    tbl->list_heads[radix_16].head = (U32)(rpt_index - 1);
    tbl->list_heads[radix_16].count -= MAX_REPEAT / 2 - 2;
    radix_16_rev = ((radix_16 >> 8) | (radix_16 << 8)) & 0xFFFF;
    tbl->list_heads[radix_16_rev].head = (U32)(rpt_index - 2);
    tbl->list_heads[radix_16_rev].count -= MAX_REPEAT / 2 - 1;
    /* Find the end */
    i += ZSTD_count(data_block + i + 2, data_block + i, data_block + block_size);
    rpt_end = i;
    /* No point if it's in the overlap region */
    if (i >= (ptrdiff_t)start) {
        U32 len = 2;
        /* Set matches at distance 2 and available length */
        for (; i >= rpt_index && len <= RADIX_MAX_LENGTH; --i) {
            SetMatchLinkAndLength(i, (U32)(i - 2), len);
            ++len;
        }
        /* Set matches at distance 2 and max length */
        for (; i >= rpt_index; --i) {
            SetMatchLinkAndLength(i, (U32)(i - 2), RADIX_MAX_LENGTH);
        }
    }
    return rpt_end;
}

/* Initialization for the reference algortithm */
#ifdef RMF_REFERENCE
static void RadixInitReference(FL2_matchTable* const tbl, const void* const data, size_t const start, size_t const end)
{
    const BYTE* const data_block = (const BYTE*)data;
    ptrdiff_t const block_size = end - 1;
    size_t st_index = 0;
    for (ptrdiff_t i = 0; i < block_size; ++i)
    {
        size_t radix_16 = ((size_t)data_block[i] << 8) | data_block[i + 1];
        U32 prev = tbl->list_heads[radix_16].head;
        if (prev != RADIX_NULL_LINK) {
            SetMatchLinkAndLength(i, prev, 2U);
            tbl->list_heads[radix_16].head = (U32)i;
            ++tbl->list_heads[radix_16].count;
        }
        else {
            SetNull(i);
            tbl->list_heads[radix_16].head = (U32)i;
            tbl->list_heads[radix_16].count = 1;
            tbl->stack[st_index++] = (U32)radix_16;
        }
    }
    SetNull(end - 1);
    tbl->end_index = (U32)st_index;
    tbl->st_index = ATOMIC_INITIAL_VALUE;
    (void)start;
}
#endif

size_t
#ifdef RMF_BITPACK
RMF_bitpackInit
#else
RMF_structuredInit
#endif
(FL2_matchTable* const tbl, const void* const data, size_t const start, size_t const end)
{
    const BYTE* const data_block = (const BYTE*)data;
    size_t st_index = 0;
    size_t radix_16;
    ptrdiff_t const block_size = end - 2;
    ptrdiff_t rpt_total = 0;
    U32 count = 0;

    if (end <= 2) {
        for (size_t i = 0; i < end; ++i) {
            SetNull(i);
        }
        return 0;
    }
#ifdef RMF_REFERENCE
    if (tbl->params.use_ref_mf) {
        RadixInitReference(tbl, data, start, end);
        return 0;
    }
#endif
    SetNull(0);
    /* Initial 2-byte radix value */
    radix_16 = ((size_t)data_block[0] << 8) | data_block[1];
    tbl->stack[st_index++] = (U32)radix_16;
    tbl->list_heads[radix_16].head = 0;
    tbl->list_heads[radix_16].count = 1;

    radix_16 = ((size_t)((BYTE)radix_16) << 8) | data_block[2];

	ptrdiff_t i = 1;
    for (; i < block_size; ++i) {
        /* Pre-load the next value for speed increase */
        size_t const next_radix = ((size_t)((BYTE)radix_16) << 8) | data_block[i + 2];

        U32 const prev = tbl->list_heads[radix_16].head;
        if (prev != RADIX_NULL_LINK) {
            S32 dist = (S32)i - prev;
            /* Check for repeat */
            if (dist > 2) {
                count = 0;
                /* Link this position to the previous occurance */
                InitMatchLink(i, prev);
                /* Set the previous to this position */
                tbl->list_heads[radix_16].head = (U32)i;
                ++tbl->list_heads[radix_16].count;
                radix_16 = next_radix;
            }
            else {
                count += 3 - dist;
                /* Do the usual if the repeat is too short */
                if (count < MAX_REPEAT - 2) {
                    InitMatchLink(i, prev);
                    tbl->list_heads[radix_16].head = (U32)i;
                    ++tbl->list_heads[radix_16].count;
                    radix_16 = next_radix;
                }
                else {
                    ptrdiff_t const prev_i = i;
                    /* Eliminate the repeat from the linked list to save time */
                    if (dist == 1) {
                        i = HandleRepeat(tbl, data_block, start, end, i, radix_16);
                        rpt_total += i - prev_i + MAX_REPEAT / 2U - 1;
                    }
                    else {
                        i = HandleRepeat2(tbl, data_block, start, end, i, radix_16);
                        rpt_total += i - prev_i + MAX_REPEAT - 2;
                    }
					if (i < block_size)
						radix_16 = ((size_t)data_block[i + 1] << 8) | data_block[i + 2];
                    count = 0;
                }
            }
        }
        else {
            count = 0;
            SetNull(i);
            tbl->list_heads[radix_16].head = (U32)i;
            tbl->list_heads[radix_16].count = 1;
            tbl->stack[st_index++] = (U32)radix_16;
            radix_16 = next_radix;
        }
    }
    /* Handle the last value */
    if (i <= block_size && tbl->list_heads[radix_16].head != RADIX_NULL_LINK) {
        SetMatchLinkAndLength(block_size, tbl->list_heads[radix_16].head, 2);
    }
    else {
        SetNull(block_size);
    }
    /* Never a match at the last byte */
    SetNull(end - 1);

    tbl->end_index = (U32)st_index;
    tbl->st_index = ATOMIC_INITIAL_VALUE;

    return rpt_total;
}

#if defined(_MSC_VER)
#  pragma warning(disable : 4701)  /* disable: C4701: potentially uninitialized local variable */
#endif


/* Copy the list into a buffer and recurse it there. This decreases cache misses and allows */
/* data characters to be loaded every fourth pass and stored for use in the next 4 passes */
static void RecurseListsBuffered(RMF_builder* const tbl,
    const BYTE* const data_block,
    size_t const block_start,
    size_t link,
    BYTE depth,
    BYTE const max_depth,
    U32 orig_list_count,
    size_t const stack_base)
{
    /* Create an offset data buffer pointer for reading the next bytes */
    const BYTE* data_src = data_block + depth;
    size_t start = 0;

    if (orig_list_count < 2 || tbl->match_buffer_limit < 2)
        return;
    do {
        size_t count = start;
        U32 list_count = (U32)(start + orig_list_count);
        U32 overlap;

        if (list_count > tbl->match_buffer_limit) {
            list_count = (U32)tbl->match_buffer_limit;
        }
        for (; count < list_count; ++count) {
            /* Pre-load next link */
            size_t const next_link = GetMatchLink(link);
            /* Get 4 data characters for later. This doesn't block on a cache miss. */
            tbl->match_buffer[count].src.u32 = MEM_read32(data_src + link);
            /* Record the actual location of this suffix */
            tbl->match_buffer[count].from = (U32)link;
            /* Initialize the next link */
            tbl->match_buffer[count].next = (U32)(count + 1) | ((U32)depth << 24);
            link = next_link;
        }
        /* Make the last element circular so pre-loading doesn't read past the end. */
        tbl->match_buffer[count - 1].next = (U32)(count - 1) | ((U32)depth << 24);
        overlap = 0;
        if (list_count < (U32)(start + orig_list_count)) {
            overlap = list_count >> MATCH_BUFFER_OVERLAP;
            overlap += !overlap;
        }
        RMF_recurseListChunk(tbl, data_block, block_start, depth, max_depth, list_count, stack_base);
        orig_list_count -= (U32)(list_count - start);
        /* Copy everything back, except the last link which never changes, and any extra overlap */
        count -= overlap + (overlap == 0);
        for (size_t index = 0; index < count; ++index) {
            size_t const from = tbl->match_buffer[index].from;
            if (from < block_start)
                return;

            {   U32 length = tbl->match_buffer[index].next >> 24;
                size_t next = tbl->match_buffer[index].next & BUFFER_LINK_MASK;
                SetMatchLinkAndLength(from, tbl->match_buffer[next].from, length);
            }
        }
        start = 0;
        if (overlap) {
            size_t dest = 0;
            for (size_t src = list_count - overlap; src < list_count; ++src) {
                tbl->match_buffer[dest].from = tbl->match_buffer[src].from;
                tbl->match_buffer[dest].src.u32 = MEM_read32(data_src + tbl->match_buffer[src].from);
                tbl->match_buffer[dest].next = (U32)(dest + 1) | ((U32)depth << 24);
                ++dest;
            }
            start = dest;
        }
    } while (orig_list_count != 0);
}

/* Parse the list with bounds checks on data reads. Stop at the point where bound checks are not required. */
/* Buffering is used so that parsing can continue below the bound to find a few matches without altering the main table. */
static void RecurseListsBound(RMF_builder* const tbl,
    const BYTE* const data_block,
    ptrdiff_t const block_size,
    RMF_tableHead* const list_head,
    U32 const max_depth)
{
    U32 list_count = list_head->count;
    ptrdiff_t link = list_head->head;
    ptrdiff_t const bounded_size = max_depth + MAX_READ_BEYOND_DEPTH;
    ptrdiff_t const bounded_start = block_size - MIN(block_size, bounded_size);
    /* Create an offset data buffer pointer for reading the next bytes */
    size_t count = 0;
    size_t extra_count = (max_depth >> 4) + 4;
    ptrdiff_t limit;
    const BYTE* data_src;
    U32 depth;
    size_t index;
    size_t st_index;
    RMF_listTail* tails_8;

    if (list_count < 2)
        return;

    list_count = MIN((U32)bounded_size, list_count);
    list_count = MIN(list_count, (U32)tbl->match_buffer_size);
    for (; count < list_count && extra_count; ++count) {
        ptrdiff_t next_link = GetMatchLink(link);
        if (link >= bounded_start) {
            --list_head->count;
            if (next_link < bounded_start) {
                list_head->head = (U32)next_link;
            }
        }
        else {
            --extra_count;
        }
        /* Record the actual location of this suffix */
        tbl->match_buffer[count].from = (U32)link;
        /* Initialize the next link */
        tbl->match_buffer[count].next = (U32)(count + 1) | ((U32)2 << 24);
        link = next_link;
    }
    list_count = (U32)count;
    limit = block_size - 2;
    data_src = data_block + 2;
    depth = 3;
    index = 0;
    st_index = 0;
    tails_8 = tbl->tails_8;
    do {
        link = tbl->match_buffer[index].from;
        if (link < limit) {
            size_t const radix_8 = data_src[link];
            /* Seen this char before? */
            const U32 prev = tails_8[radix_8].prev_index;
            if (prev != RADIX_NULL_LINK) {
                ++tails_8[radix_8].list_count;
                /* Link the previous occurrence to this one and record the new length */
                tbl->match_buffer[prev].next = (U32)index | (depth << 24);
            }
            else {
                tails_8[radix_8].list_count = 1;
                /* Add the new sub list to the stack */
                tbl->stack[st_index].head = (U32)index;
                /* This will be converted to a count at the end */
                tbl->stack[st_index].count = (U32)radix_8;
                ++st_index;
            }
            tails_8[radix_8].prev_index = (U32)index;
        }
        ++index;
    } while (index < list_count);
    /* Convert radix values on the stack to counts and reset any used tail slots */
    for (size_t j = 0; j < st_index; ++j) {
        tails_8[tbl->stack[j].count].prev_index = RADIX_NULL_LINK;
        tbl->stack[j].count = tails_8[tbl->stack[j].count].list_count;
    }
    while (st_index > 0) {
        size_t prev_st_index;

        /* Pop an item off the stack */
        --st_index;
        list_count = tbl->stack[st_index].count;
        if (list_count < 2) {
            /* Nothing to match with */
            continue;
        }
        index = tbl->stack[st_index].head;
        depth = (tbl->match_buffer[index].next >> 24);
        if (depth >= max_depth)
            continue;
        link = tbl->match_buffer[index].from;
        if (link < bounded_start) {
            /* Chain starts before the bounded region */
            continue;
        }
        data_src = data_block + depth;
        limit = block_size - depth;
        ++depth;
        prev_st_index = st_index;
        do {
            link = tbl->match_buffer[index].from;
            if (link < limit) {
                size_t const radix_8 = data_src[link];
                U32 const prev = tails_8[radix_8].prev_index;
                if (prev != RADIX_NULL_LINK) {
                    ++tails_8[radix_8].list_count;
                    tbl->match_buffer[prev].next = (U32)index | ((U32)depth << 24);
                }
                else {
                    tails_8[radix_8].list_count = 1;
                    tbl->stack[st_index].head = (U32)index;
                    tbl->stack[st_index].count = (U32)radix_8;
                    ++st_index;
                }
                tails_8[radix_8].prev_index = (U32)index;
            }
            index = tbl->match_buffer[index].next & BUFFER_LINK_MASK;
        } while (--list_count != 0);
        for (size_t j = prev_st_index; j < st_index; ++j) {
            tails_8[tbl->stack[j].count].prev_index = RADIX_NULL_LINK;
            tbl->stack[j].count = tails_8[tbl->stack[j].count].list_count;
        }
    }
    /* Copy everything back above the bound */
    --count;
    for (index = 0; index < count; ++index) {
        ptrdiff_t const from = tbl->match_buffer[index].from;
        size_t next;
        U32 length;

        if (from < bounded_start)
            break;
        length = tbl->match_buffer[index].next >> 24;
        length = MIN(length, (U32)(block_size - from));
        next = tbl->match_buffer[index].next & BUFFER_LINK_MASK;
        SetMatchLinkAndLength(from, tbl->match_buffer[next].from, length);
    }
}

/* Compare each string with all others to find the best match */
static void BruteForce(RMF_builder* const tbl,
    const BYTE* const data_block,
    size_t const block_start,
    size_t link,
    size_t const list_count,
    U32 const depth,
    U32 const max_depth)
{
    const BYTE* data_src = data_block + depth;
    size_t buffer[MAX_BRUTE_FORCE_LIST_SIZE + 1];
    size_t const limit = max_depth - depth;
    size_t i = 1;

    buffer[0] = link;
    /* Pre-load all locations */
    do {
        link = GetMatchLink(link);
        buffer[i] = link;
    } while (++i < list_count);
    i = 0;
    do {
        size_t longest = 0;
        size_t j = i + 1;
        size_t longest_index = j;
        const BYTE* const data = data_src + buffer[i];
        do {
            const BYTE* data_2 = data_src + buffer[j];
            size_t len_test = 0;
            while (data[len_test] == data_2[len_test] && len_test < limit) {
                ++len_test;
            }
            if (len_test > longest) {
                longest_index = j;
                longest = len_test;
                if (len_test >= limit) {
                    break;
                }
            }
        } while (++j < list_count);
        if (longest > 0) {
            SetMatchLinkAndLength(buffer[i],
                (U32)buffer[longest_index],
                depth + (U32)longest);
        }
        ++i;
    } while (i < list_count - 1 && buffer[i] >= block_start);
}

static void RecurseLists16(RMF_builder* const tbl,
    const BYTE* const data_block,
    size_t const block_start,
    size_t link,
    U32 count,
    U32 const max_depth)
{
    /* Offset data pointer. This method is only called at depth 2 */
    const BYTE* const data_src = data_block + 2;
    /* Load radix values from the data chars */
    size_t next_radix_8 = data_src[link];
    size_t next_radix_16 = next_radix_8 + ((size_t)(data_src[link + 1]) << 8);
    size_t reset_list[RADIX8_TABLE_SIZE];
    size_t reset_count = 0;
    size_t st_index = 0;
    U32 prev;
    /* Last one is done separately */
    --count;
    do
    {
        /* Pre-load the next link */
        size_t const next_link = GetInitialMatchLink(link);
        size_t const radix_8 = next_radix_8;
        size_t const radix_16 = next_radix_16;
        /* Initialization doesn't set lengths to 2 because it's a waste of time if buffering is used */
        SetMatchLength(link, (U32)next_link, 2);

        next_radix_8 = data_src[next_link];
        next_radix_16 = next_radix_8 + ((size_t)(data_src[next_link + 1]) << 8);

        prev = tbl->tails_8[radix_8].prev_index;
        if (prev != RADIX_NULL_LINK) {
            /* Link the previous occurrence to this one at length 3. */
            /* This will be overwritten if a 4 is found. */
            SetMatchLinkAndLength(prev, (U32)link, 3);
        }
        else {
            reset_list[reset_count++] = radix_8;
        }
        tbl->tails_8[radix_8].prev_index = (U32)link;

        prev = tbl->tails_16[radix_16].prev_index;
        if (prev != RADIX_NULL_LINK) {
            ++tbl->tails_16[radix_16].list_count;
            /* Link at length 4, overwriting the 3 */
            SetMatchLinkAndLength(prev, (U32)link, 4);
        }
        else {
            tbl->tails_16[radix_16].list_count = 1;
            tbl->stack[st_index].head = (U32)link;
            tbl->stack[st_index].count = (U32)radix_16;
            ++st_index;
        }
        tbl->tails_16[radix_16].prev_index = (U32)link;
        link = next_link;
    } while (--count > 0);
    /* Do the last location */
    prev = tbl->tails_8[next_radix_8].prev_index;
    if (prev != RADIX_NULL_LINK) {
        SetMatchLinkAndLength(prev, (U32)link, 3);
    }
    prev = tbl->tails_16[next_radix_16].prev_index;
    if (prev != RADIX_NULL_LINK) {
        ++tbl->tails_16[next_radix_16].list_count;
        SetMatchLinkAndLength(prev, (U32)link, 4);
    }
    for (size_t i = 0; i < reset_count; ++i) {
        tbl->tails_8[reset_list[i]].prev_index = RADIX_NULL_LINK;
    }
    for (size_t i = 0; i < st_index; ++i) {
        tbl->tails_16[tbl->stack[i].count].prev_index = RADIX_NULL_LINK;
        tbl->stack[i].count = tbl->tails_16[tbl->stack[i].count].list_count;
    }
    while (st_index > 0) {
        U32 list_count;
        U32 depth;

        --st_index;
        list_count = tbl->stack[st_index].count;
        if (list_count < 2) {
            /* Nothing to do */
            continue;
        }
        link = tbl->stack[st_index].head;
        if (link < block_start)
            continue;
        if (st_index > STACK_SIZE - RADIX16_TABLE_SIZE
            && st_index > STACK_SIZE - list_count)
        {
            /* Potential stack overflow. Rare. */
            continue;
        }
        /* The current depth */
        depth = GetMatchLength(link);
        if (list_count <= MAX_BRUTE_FORCE_LIST_SIZE) {
            /* Quicker to use brute force, each string compared with all previous strings */
            BruteForce(tbl, data_block,
                block_start,
                link,
                list_count,
                depth,
                max_depth);
            continue;
        }
        /* Send to the buffer at depth 4 */
        RecurseListsBuffered(tbl,
            data_block,
            block_start,
            link,
            (BYTE)depth,
            (BYTE)max_depth,
            list_count,
            st_index);
    }
}

#if 0
static void RecurseListsUnbuf16(RMF_builder* const tbl,
    const BYTE* const data_block,
    size_t const block_start,
    size_t link,
    U32 count,
    U32 const max_depth)
{
    /* Offset data pointer. This method is only called at depth 2 */
    const BYTE* data_src = data_block + 2;
    /* Load radix values from the data chars */
    size_t next_radix_8 = data_src[link];
    size_t next_radix_16 = next_radix_8 + ((size_t)(data_src[link + 1]) << 8);
    RMF_listTail* tails_8 = tbl->tails_8;
    size_t reset_list[RADIX8_TABLE_SIZE];
    size_t reset_count = 0;
    size_t st_index = 0;
    U32 prev;
    /* Last one is done separately */
    --count;
    do
    {
        /* Pre-load the next link */
        size_t next_link = GetInitialMatchLink(link);
        /* Initialization doesn't set lengths to 2 because it's a waste of time if buffering is used */
        SetMatchLength(link, (U32)next_link, 2);
        size_t radix_8 = next_radix_8;
        size_t radix_16 = next_radix_16;
        next_radix_8 = data_src[next_link];
        next_radix_16 = next_radix_8 + ((size_t)(data_src[next_link + 1]) << 8);
        prev = tails_8[radix_8].prev_index;
        if (prev != RADIX_NULL_LINK) {
            /* Link the previous occurrence to this one at length 3. */
            /* This will be overwritten if a 4 is found. */
            SetMatchLinkAndLength(prev, (U32)link, 3);
        }
        else {
            reset_list[reset_count++] = radix_8;
        }
        tails_8[radix_8].prev_index = (U32)link;
        prev = tbl->tails_16[radix_16].prev_index;
        if (prev != RADIX_NULL_LINK) {
            ++tbl->tails_16[radix_16].list_count;
            /* Link at length 4, overwriting the 3 */
            SetMatchLinkAndLength(prev, (U32)link, 4);
        }
        else {
            tbl->tails_16[radix_16].list_count = 1;
            tbl->stack[st_index].head = (U32)link;
            tbl->stack[st_index].count = (U32)radix_16;
            ++st_index;
        }
        tbl->tails_16[radix_16].prev_index = (U32)link;
        link = next_link;
    } while (--count > 0);
    /* Do the last location */
    prev = tails_8[next_radix_8].prev_index;
    if (prev != RADIX_NULL_LINK) {
        SetMatchLinkAndLength(prev, (U32)link, 3);
    }
    prev = tbl->tails_16[next_radix_16].prev_index;
    if (prev != RADIX_NULL_LINK) {
        ++tbl->tails_16[next_radix_16].list_count;
        SetMatchLinkAndLength(prev, (U32)link, 4);
    }
    for (size_t i = 0; i < reset_count; ++i) {
        tails_8[reset_list[i]].prev_index = RADIX_NULL_LINK;
    }
    reset_count = 0;
    for (size_t i = 0; i < st_index; ++i) {
        tbl->tails_16[tbl->stack[i].count].prev_index = RADIX_NULL_LINK;
        tbl->stack[i].count = tbl->tails_16[tbl->stack[i].count].list_count;
    }
    while (st_index > 0) {
        --st_index;
        U32 list_count = tbl->stack[st_index].count;
        if (list_count < 2) {
            /* Nothing to do */
            continue;
        }
        link = tbl->stack[st_index].head;
        if (link < block_start)
            continue;
        if (st_index > STACK_SIZE - RADIX16_TABLE_SIZE
            && st_index > STACK_SIZE - list_count)
        {
            /* Potential stack overflow. Rare. */
            continue;
        }
        /* The current depth */
        U32 depth = GetMatchLength(link);
        if (list_count <= MAX_BRUTE_FORCE_LIST_SIZE) {
            /* Quicker to use brute force, each string compared with all previous strings */
            BruteForce(tbl, data_block,
                block_start,
                link,
                list_count,
                depth,
                max_depth);
            continue;
        }
        const BYTE* data_src = data_block + depth;
        size_t next_radix_8 = data_src[link];
        size_t next_radix_16 = next_radix_8 + ((size_t)(data_src[link + 1]) << 8);
        /* Next depth for 1 extra char */
        ++depth;
        /* and for 2 */
        U32 depth_2 = depth + 1;
        size_t prev_st_index = st_index;
        /* Last location is done separately */
        --list_count;
        /* Last pass is done separately. Both of these values are always even. */
        if (depth_2 < max_depth) {
            do {
                size_t radix_8 = next_radix_8;
                size_t radix_16 = next_radix_16;
                size_t next_link = GetMatchLink(link);
                next_radix_8 = data_src[next_link];
                next_radix_16 = next_radix_8 + ((size_t)(data_src[next_link + 1]) << 8);
                size_t prev = tbl->tails_8[radix_8].prev_index;
                if (prev != RADIX_NULL_LINK) {
                    /* Odd numbered match length, will be overwritten if 2 chars are matched */
                    SetMatchLinkAndLength(prev, (U32)(link), depth);
                }
                else {
                    reset_list[reset_count++] = radix_8;
                }
                tbl->tails_8[radix_8].prev_index = (U32)link;
                prev = tbl->tails_16[radix_16].prev_index;
                if (prev != RADIX_NULL_LINK) {
                    ++tbl->tails_16[radix_16].list_count;
                    SetMatchLinkAndLength(prev, (U32)(link), depth_2);
                }
                else {
                    tbl->tails_16[radix_16].list_count = 1;
                    tbl->stack[st_index].head = (U32)(link);
                    tbl->stack[st_index].count = (U32)(radix_16);
                    ++st_index;
                }
                tbl->tails_16[radix_16].prev_index = (U32)(link);
                link = next_link;
            } while (--list_count != 0);
            size_t prev = tbl->tails_8[next_radix_8].prev_index;
            if (prev != RADIX_NULL_LINK) {
                SetMatchLinkAndLength(prev, (U32)(link), depth);
            }
            prev = tbl->tails_16[next_radix_16].prev_index;
            if (prev != RADIX_NULL_LINK) {
                ++tbl->tails_16[next_radix_16].list_count;
                SetMatchLinkAndLength(prev, (U32)(link), depth_2);
            }
            for (size_t i = prev_st_index; i < st_index; ++i) {
                tbl->tails_16[tbl->stack[i].count].prev_index = RADIX_NULL_LINK;
                tbl->stack[i].count = tbl->tails_16[tbl->stack[i].count].list_count;
            }
            for (size_t i = 0; i < reset_count; ++i) {
                tails_8[reset_list[i]].prev_index = RADIX_NULL_LINK;
            }
            reset_count = 0;
        }
        else {
            do {
                size_t radix_8 = next_radix_8;
                size_t radix_16 = next_radix_16;
                size_t next_link = GetMatchLink(link);
                next_radix_8 = data_src[next_link];
                next_radix_16 = next_radix_8 + ((size_t)(data_src[next_link + 1]) << 8);
                size_t prev = tbl->tails_8[radix_8].prev_index;
                if (prev != RADIX_NULL_LINK) {
                    SetMatchLinkAndLength(prev, (U32)(link), depth);
                }
                else {
                    reset_list[reset_count++] = radix_8;
                }
                tbl->tails_8[radix_8].prev_index = (U32)link;
                prev = tbl->tails_16[radix_16].prev_index;
                if (prev != RADIX_NULL_LINK) {
                    SetMatchLinkAndLength(prev, (U32)(link), depth_2);
                }
                else {
                    tbl->stack[st_index].count = (U32)radix_16;
                    ++st_index;
                }
                tbl->tails_16[radix_16].prev_index = (U32)(link);
                link = next_link;
            } while (--list_count != 0);
            size_t prev = tbl->tails_8[next_radix_8].prev_index;
            if (prev != RADIX_NULL_LINK) {
                SetMatchLinkAndLength(prev, (U32)(link), depth);
            }
            prev = tbl->tails_16[next_radix_16].prev_index;
            if (prev != RADIX_NULL_LINK) {
                SetMatchLinkAndLength(prev, (U32)(link), depth_2);
            }
            for (size_t i = prev_st_index; i < st_index; ++i) {
                tbl->tails_16[tbl->stack[i].count].prev_index = RADIX_NULL_LINK;
            }
            st_index = prev_st_index;
            for (size_t i = 0; i < reset_count; ++i) {
                tails_8[reset_list[i]].prev_index = RADIX_NULL_LINK;
            }
            reset_count = 0;
        }
    }
}
#endif

#ifdef RMF_REFERENCE

/* Simple, slow, complete parsing for reference */
static void RecurseListsReference(RMF_builder* const tbl,
    const BYTE* const data_block,
    size_t const block_size,
    size_t link,
    U32 count,
    U32 const max_depth)
{
    /* Offset data pointer. This method is only called at depth 2 */
    const BYTE* data_src = data_block + 2;
    size_t limit = block_size - 2;
    size_t st_index = 0;

    do
    {
        if (link < limit) {
            size_t const radix_8 = data_src[link];
            size_t const prev = tbl->tails_8[radix_8].prev_index;
            if (prev != RADIX_NULL_LINK) {
                ++tbl->tails_8[radix_8].list_count;
                SetMatchLinkAndLength(prev, (U32)link, 3);
            }
            else {
                tbl->tails_8[radix_8].list_count = 1;
                tbl->stack[st_index].head = (U32)link;
                tbl->stack[st_index].count = (U32)radix_8;
                ++st_index;
            }
            tbl->tails_8[radix_8].prev_index = (U32)link;
        }
        link = GetMatchLink(link);
    } while (--count > 0);
    for (size_t i = 0; i < st_index; ++i) {
        tbl->stack[i].count = tbl->tails_8[tbl->stack[i].count].list_count;
    }
    memset(tbl->tails_8, 0xFF, sizeof(tbl->tails_8));
    while (st_index > 0) {
        U32 list_count;
        U32 depth;
        size_t prev_st_index;

        --st_index;
        list_count = tbl->stack[st_index].count;
        if (list_count < 2) {
            /* Nothing to do */
            continue;
        }
        if (st_index > STACK_SIZE - RADIX8_TABLE_SIZE
            && st_index > STACK_SIZE - list_count)
        {
            /* Potential stack overflow. Rare. */
            continue;
        }
        link = tbl->stack[st_index].head;
        /* The current depth */
        depth = GetMatchLength(link);
        if (depth >= max_depth)
            continue;
        data_src = data_block + depth;
        limit = block_size - depth;
        /* Next depth for 1 extra char */
        ++depth;
        prev_st_index = st_index;
        do {
            if (link < limit) {
                size_t const radix_8 = data_src[link];
                size_t const prev = tbl->tails_8[radix_8].prev_index;
                if (prev != RADIX_NULL_LINK) {
                    ++tbl->tails_8[radix_8].list_count;
                    SetMatchLinkAndLength(prev, (U32)link, depth);
                }
                else {
                    tbl->tails_8[radix_8].list_count = 1;
                    tbl->stack[st_index].head = (U32)link;
                    tbl->stack[st_index].count = (U32)radix_8;
                    ++st_index;
                }
                tbl->tails_8[radix_8].prev_index = (U32)link;
            }
            link = GetMatchLink(link);
        } while (--list_count != 0);
        for (size_t i = prev_st_index; i < st_index; ++i) {
            tbl->stack[i].count = tbl->tails_8[tbl->stack[i].count].list_count;
        }
        memset(tbl->tails_8, 0xFF, sizeof(tbl->tails_8));
    }
}

#endif /* RMF_REFERENCE */

/* Atomically take a list from the head table */
static ptrdiff_t RMF_getNextList(FL2_matchTable* const tbl, unsigned const multi_thread)
{
    if (tbl->st_index < tbl->end_index) {
        long index = multi_thread ? FL2_atomic_increment(tbl->st_index) : FL2_nonAtomic_increment(tbl->st_index);
        if (index < tbl->end_index) {
            return index;
        }
    }
    return -1;
}

#define UPDATE_INTERVAL 0x40000U

/* Iterate the head table concurrently with other threads, and recurse each list until max_depth is reached */
int
#ifdef RMF_BITPACK
RMF_bitpackBuildTable
#else
RMF_structuredBuildTable
#endif
(FL2_matchTable* const tbl,
	size_t const job,
    unsigned const multi_thread,
    FL2_dataBlock const block,
    FL2_progressFn progress, void* opaque, U32 weight, size_t init_done)
{
    if (!block.end)
        return 0;
    U64 const enc_size = block.end - block.start;
    unsigned const best = !tbl->params.divide_and_conquer;
    unsigned const max_depth = MIN(tbl->params.depth, RADIX_MAX_LENGTH) & ~1;
    size_t const bounded_start = block.end - max_depth - MAX_READ_BEYOND_DEPTH;
    ptrdiff_t next_progress = 0;
    size_t update = UPDATE_INTERVAL;
    size_t total = init_done;

    for (;;)
    {
        /* Get the next to process */
        ptrdiff_t index = RMF_getNextList(tbl, multi_thread);
        RMF_tableHead list_head;

        if (index < 0) {
            break;
        }
        if (progress) {
            while (next_progress < index) {
                total += tbl->list_heads[tbl->stack[next_progress]].count;
                ++next_progress;
            }
            if (total >= update) {
                if (progress((size_t)((total * enc_size / block.end * weight) >> 4), opaque)) {
					FL2_atomic_add(tbl->st_index, RADIX16_TABLE_SIZE);
                    return 1;
                }
                update = total + UPDATE_INTERVAL;
            }
        }
        index = tbl->stack[index];
        list_head = tbl->list_heads[index];
        tbl->list_heads[index].head = RADIX_NULL_LINK;
        if (list_head.count < 2 || list_head.head < block.start) {
            continue;
        }
#ifdef RMF_REFERENCE
        if (tbl->params.use_ref_mf) {
            RecurseListsReference(tbl->builders[job], block.data, block.end, list_head.head, list_head.count, max_depth);
            continue;
        }
#endif
        if (list_head.head >= bounded_start) {
            RecurseListsBound(tbl->builders[job], block.data, block.end, &list_head, (BYTE)max_depth);
            if (list_head.count < 2 || list_head.head < block.start) {
                continue;
            }
        }
        if (best && list_head.count > tbl->builders[job]->match_buffer_limit)
        {
            /* Not worth buffering or too long */
            RecurseLists16(tbl->builders[job], block.data, block.start, list_head.head, list_head.count, max_depth);
        }
        else {
            RecurseListsBuffered(tbl->builders[job], block.data, block.start, list_head.head, 2, (BYTE)max_depth, list_head.count, 0);
        }
    }
    return 0;
}

int
#ifdef RMF_BITPACK
RMF_bitpackIntegrityCheck
#else
RMF_structuredIntegrityCheck
#endif
(const FL2_matchTable* const tbl, const BYTE* const data, size_t index, size_t const end, unsigned const max_depth)
{
    int err = 0;
    for (index += !index; index < end; ++index) {
        U32 link;
        U32 length;
        U32 len_test;
        U32 limit;

        if (IsNull(index))
            continue;
        link = GetMatchLink(index);
        if (link >= index) {
            printf("Forward link at %X to %u\r\n", (U32)index, link);
            err = 1;
            continue;
        }
        length = GetMatchLength(index);
        if (index && length < RADIX_MAX_LENGTH && link - 1 == GetMatchLink(index - 1) && length + 1 == GetMatchLength(index - 1))
            continue;
        len_test = 0;
        limit = MIN((U32)(end - index), RADIX_MAX_LENGTH);
        for (; len_test < limit && data[link + len_test] == data[index + len_test]; ++len_test) {
        }
        if (len_test < length) {
            printf("Failed integrity check: pos %X, length %u, actual %u\r\n", (U32)index, length, len_test);
            err = 1;
        }
        if (length < max_depth && len_test > length)
            printf("Shortened match at %X: %u of %u\r\n", (U32)index, length, len_test);
    }
    return err;
}


static size_t ExtendMatch(const FL2_matchTable* const tbl,
    const BYTE* const data,
    ptrdiff_t const start_index,
    ptrdiff_t const limit,
    U32 const link,
    size_t const length)
{
    ptrdiff_t end_index = start_index + length;
    ptrdiff_t const dist = start_index - link;
    while (end_index < limit && end_index - (ptrdiff_t)GetMatchLink(end_index) == dist) {
        end_index += GetMatchLength(end_index);
    }
    if (end_index >= limit) {
        return limit - start_index;
    }
    while (end_index < limit && data[end_index - dist] == data[end_index]) {
        ++end_index;
    }
    return end_index - start_index;
}

size_t
#ifdef RMF_BITPACK
RMF_bitpackGetMatch
#else
RMF_structuredGetMatch
#endif
(const FL2_matchTable* const tbl,
    const BYTE* const data,
    size_t const index,
    size_t const limit,
    unsigned const max_depth,
    size_t* const offset_ptr)
{
    size_t length;
    size_t dist;
    U32 link;
    if (IsNull(index))
        return 0;
    link = GetMatchLink(index);
    length = GetMatchLength(index);
    if (length < 2)
        return 0;
    dist = index - link;
    *offset_ptr = dist;
    if (length > limit - index)
        return limit - index;
    if (length == max_depth
        || length == RADIX_MAX_LENGTH /* from HandleRepeat */)
    {
        length = ExtendMatch(tbl, data, index, limit, link, length);
    }
    return length;
}
