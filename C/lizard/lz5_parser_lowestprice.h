#define LZ5_LOWESTPRICE_MIN_OFFSET 8


FORCE_INLINE size_t LZ5_more_profitable(LZ5_stream_t* const ctx, const BYTE *best_ip, size_t best_off, size_t best_common, const BYTE *ip, size_t off, size_t common, size_t literals, int last_off)
{
    size_t sum;

    if (literals > 0)
        sum = MAX(common + literals, best_common);
    else
        sum = MAX(common, best_common - literals);

    if ((int)off == last_off) off = 0; // rep code
    if ((int)best_off == last_off) best_off = 0;

    return LZ5_get_price_LZ5v2(ctx, last_off, ip, ctx->off24pos, sum - common, (U32)off, common) <= LZ5_get_price_LZ5v2(ctx, last_off, best_ip, ctx->off24pos, sum - best_common, (U32)best_off, best_common);
} 


FORCE_INLINE size_t LZ5_better_price(LZ5_stream_t* const ctx, const BYTE *best_ip, size_t best_off, size_t best_common, const BYTE *ip, size_t off, size_t common, int last_off)
{
    if ((int)off == last_off) off = 0; // rep code
    if ((int)best_off == last_off) best_off = 0;

    return LZ5_get_price_LZ5v2(ctx, last_off, ip, ctx->off24pos, 0, (U32)off, common) < LZ5_get_price_LZ5v2(ctx, last_off, best_ip, ctx->off24pos, common - best_common, (U32)best_off, best_common);
}


FORCE_INLINE int LZ5_FindMatchLowestPrice (LZ5_stream_t* ctx,   /* Index table will be updated */
                                               const BYTE* ip, const BYTE* const iLimit,
                                               const BYTE** matchpos)
{
    U32* const chainTable = ctx->chainTable;
    U32* const HashTable = ctx->hashTable;
    const BYTE* const base = ctx->base;
    const BYTE* const dictBase = ctx->dictBase;
    const intptr_t dictLimit = ctx->dictLimit;
    const BYTE* const dictEnd = dictBase + dictLimit;
    const intptr_t maxDistance = (1 << ctx->params.windowLog) - 1;
    const intptr_t current = (ip - base);
    const intptr_t lowLimit = ((intptr_t)ctx->lowLimit + maxDistance >= current) ? (intptr_t)ctx->lowLimit : current - maxDistance;
    const BYTE* const lowPrefixPtr = base + dictLimit;
    const U32 contentMask = (1 << ctx->params.contentLog) - 1;
    const size_t minMatchLongOff = ctx->params.minMatchLongOff;
    intptr_t matchIndex;
    const BYTE* match, *matchDict;
    int nbAttempts=ctx->params.searchNum;
    size_t ml=0, mlt;

    matchIndex = HashTable[LZ5_hashPtr(ip, ctx->params.hashLog, ctx->params.searchLength)];

    if (ctx->last_off >= LZ5_LOWESTPRICE_MIN_OFFSET) {
        intptr_t matchIndexLO = (ip - ctx->last_off) - base;
        if (matchIndexLO >= lowLimit) {
            if (matchIndexLO >= dictLimit) {
                match = base + matchIndexLO;
                mlt = LZ5_count(ip, match, iLimit);// + MINMATCH;
          //      if ((mlt >= minMatchLongOff) || (ctx->last_off < LZ5_MAX_16BIT_OFFSET))
                if (mlt > REPMINMATCH) {
                    *matchpos = match;
                    return (int)mlt;
                }
            } else {
                match = dictBase + matchIndexLO;
                if ((U32)((dictLimit-1) - matchIndexLO) >= 3) {  /* intentional overflow */
                    mlt = LZ5_count_2segments(ip, match, iLimit, dictEnd, lowPrefixPtr);
                 //   if ((mlt >= minMatchLongOff) || (ctx->last_off < LZ5_MAX_16BIT_OFFSET)) 
                    if (mlt > REPMINMATCH) {
                        *matchpos = base + matchIndexLO;  /* virtual matchpos */
                        return (int)mlt;
                    }
               }
            }
        }
    }


#if MINMATCH == 3
    {
        U32 matchIndex3 = ctx->hashTable3[LZ5_hash3Ptr(ip, ctx->params.hashLog3)];
        if (matchIndex3 < current && matchIndex3 >= lowLimit)
        {
            size_t offset = (size_t)current - matchIndex3;
            if (offset < LZ5_MAX_8BIT_OFFSET)
            {
                match = ip - offset;
                if (match > base && MEM_readMINMATCH(ip) == MEM_readMINMATCH(match))
                {
                    ml = 3;//LZ5_count(ip+MINMATCH, match+MINMATCH, iLimit) + MINMATCH;
                    *matchpos = match;
                }
            }
        }
    }
#endif
    while ((matchIndex < current) && (matchIndex >= lowLimit) && (nbAttempts)) {
        nbAttempts--;
        match = base + matchIndex;
        if ((U32)(ip - match) >= LZ5_LOWESTPRICE_MIN_OFFSET) {
            if (matchIndex >= dictLimit) {
                if (*(match+ml) == *(ip+ml) && (MEM_read32(match) == MEM_read32(ip))) {
                    mlt = LZ5_count(ip+MINMATCH, match+MINMATCH, iLimit) + MINMATCH;
                    if ((mlt >= minMatchLongOff) || ((U32)(ip - match) < LZ5_MAX_16BIT_OFFSET))
                    if (!ml || (mlt > ml && LZ5_better_price(ctx, ip, (ip - *matchpos), ml, ip, (ip - match), mlt, ctx->last_off)))
                    { ml = mlt; *matchpos = match; }
                }
            } else {
                matchDict = dictBase + matchIndex;
    //            fprintf(stderr, "dictBase[%p]+matchIndex[%d]=match[%p] dictLimit=%d base=%p ip=%p iLimit=%p off=%d\n", dictBase, matchIndex, match, dictLimit, base, ip, iLimit, (U32)(ip-match));
                if ((U32)((dictLimit-1) - matchIndex) >= 3)  /* intentional overflow */
                if (MEM_read32(matchDict) == MEM_read32(ip)) {
                    mlt = LZ5_count_2segments(ip+MINMATCH, matchDict+MINMATCH, iLimit, dictEnd, lowPrefixPtr) + MINMATCH;
                    if ((mlt >= minMatchLongOff) || ((U32)(ip - match) < LZ5_MAX_16BIT_OFFSET))
                    if (!ml || (mlt > ml && LZ5_better_price(ctx, ip, (ip - *matchpos), ml, ip, (U32)(ip - match), mlt, ctx->last_off)))
                    { ml = mlt; *matchpos = match; }   /* virtual matchpos */
                }
            }
        }
        matchIndex -= chainTable[matchIndex & contentMask];
    }

    return (int)ml;
}


FORCE_INLINE size_t LZ5_GetWiderMatch (
    LZ5_stream_t* ctx,
    const BYTE* const ip,
    const BYTE* const iLowLimit,
    const BYTE* const iHighLimit,
    size_t longest,
    const BYTE** matchpos,
    const BYTE** startpos)
{
    U32* const chainTable = ctx->chainTable;
    U32* const HashTable = ctx->hashTable;
    const BYTE* const base = ctx->base;
    const BYTE* const dictBase = ctx->dictBase;
    const intptr_t dictLimit = ctx->dictLimit;
    const BYTE* const dictEnd = dictBase + dictLimit;
    const intptr_t maxDistance = (1 << ctx->params.windowLog) - 1;
    const intptr_t current = (ip - base);
    const intptr_t lowLimit = ((intptr_t)ctx->lowLimit + maxDistance >= current) ? (intptr_t)ctx->lowLimit : current - maxDistance;
    const BYTE* const lowPrefixPtr = base + dictLimit;
    const U32 contentMask = (1 << ctx->params.contentLog) - 1;
    const BYTE* match, *matchDict;
    const size_t minMatchLongOff = ctx->params.minMatchLongOff;
    intptr_t matchIndex;
    int nbAttempts = ctx->params.searchNum;
    size_t mlt;

    /* First Match */
    matchIndex = HashTable[LZ5_hashPtr(ip, ctx->params.hashLog, ctx->params.searchLength)];

    if (ctx->last_off >= LZ5_LOWESTPRICE_MIN_OFFSET) {
        intptr_t matchIndexLO = (ip - ctx->last_off) - base;
        if (matchIndexLO >= lowLimit) {
            if (matchIndexLO >= dictLimit) {
                match = base + matchIndexLO;
                if (MEM_readMINMATCH(match) == MEM_readMINMATCH(ip)) {
                    int back = 0;
                    mlt = LZ5_count(ip+MINMATCH, match+MINMATCH, iHighLimit) + MINMATCH;
                    while ((ip+back > iLowLimit) && (match+back > lowPrefixPtr) && (ip[back-1] == match[back-1])) back--;
                    mlt -= back;

                    if (mlt > longest)
                    if ((mlt >= minMatchLongOff) || (ctx->last_off < LZ5_MAX_16BIT_OFFSET)) {
                        *matchpos = match+back;
                        *startpos = ip+back;
                        longest = mlt;
                    }
                }
            } else {
                match = dictBase + matchIndexLO;
                if ((U32)((dictLimit-1) - matchIndexLO) >= 3)  /* intentional overflow */
                if (MEM_readMINMATCH(match) == MEM_readMINMATCH(ip)) {
                    int back=0;
                    mlt = LZ5_count_2segments(ip+MINMATCH, match+MINMATCH, iHighLimit, dictEnd, lowPrefixPtr) + MINMATCH;
                    while ((ip+back > iLowLimit) && (matchIndexLO+back > lowLimit) && (ip[back-1] == match[back-1])) back--;
                    mlt -= back;

                    if (mlt > longest)
                    if ((mlt >= minMatchLongOff) || (ctx->last_off < LZ5_MAX_16BIT_OFFSET)) {
                        *matchpos = base + matchIndexLO + back;  /* virtual matchpos */
                        *startpos = ip+back;
                        longest = mlt;
                    }
                }
            }
        }
    }

#if MINMATCH == 3
    {
        U32 matchIndex3 = ctx->hashTable3[LZ5_hash3Ptr(ip, ctx->params.hashLog3)];
        if (matchIndex3 < current && matchIndex3 >= lowLimit) {
            size_t offset = (size_t)current - matchIndex3;
            if (offset < LZ5_MAX_8BIT_OFFSET) {
                match = ip - offset;
                if (match > base && MEM_readMINMATCH(ip) == MEM_readMINMATCH(match)) {
                    mlt = LZ5_count(ip + MINMATCH, match + MINMATCH, iHighLimit) + MINMATCH;

                    int back = 0;
                    while ((ip + back > iLowLimit) && (match + back > lowPrefixPtr) && (ip[back - 1] == match[back - 1])) back--;
                    mlt -= back;

                    if (!longest || (mlt > longest && LZ5_better_price(ctx, *startpos, (*startpos - *matchpos), longest, ip, (ip - match), mlt, ctx->last_off))) {
                        *matchpos = match + back;
                        *startpos = ip + back;
                        longest = mlt;
                    }
                }
            }
        }
    }
#endif

    while ((matchIndex < current) && (matchIndex >= lowLimit) && (nbAttempts)) {
        nbAttempts--;
        match = base + matchIndex;
        if ((U32)(ip - match) >= LZ5_LOWESTPRICE_MIN_OFFSET) {
            if (matchIndex >= dictLimit) {
                if (MEM_read32(match) == MEM_read32(ip)) {
                    int back = 0;
                    mlt = LZ5_count(ip+MINMATCH, match+MINMATCH, iHighLimit) + MINMATCH;
                    while ((ip+back > iLowLimit) && (match+back > lowPrefixPtr) && (ip[back-1] == match[back-1])) back--;
                    mlt -= back;

                    if ((mlt >= minMatchLongOff) || ((U32)(ip - match) < LZ5_MAX_16BIT_OFFSET))
                    if (!longest || (mlt > longest && LZ5_better_price(ctx, *startpos, (*startpos - *matchpos), longest, ip, (ip - match), mlt, ctx->last_off)))
                    { longest = mlt; *startpos = ip+back; *matchpos = match+back; }
                }
            } else {
                matchDict = dictBase + matchIndex;
    //            fprintf(stderr, "dictBase[%p]+matchIndex[%d]=match[%p] dictLimit=%d base=%p ip=%p iLimit=%p off=%d\n", dictBase, matchIndex, match, dictLimit, base, ip, iLimit, (U32)(ip-match));
                if ((U32)((dictLimit-1) - matchIndex) >= 3)  /* intentional overflow */
                if (MEM_read32(matchDict) == MEM_read32(ip)) {
                    int back=0;
                    mlt = LZ5_count_2segments(ip+MINMATCH, matchDict+MINMATCH, iHighLimit, dictEnd, lowPrefixPtr) + MINMATCH;
                    while ((ip+back > iLowLimit) && (matchIndex+back > lowLimit) && (ip[back-1] == matchDict[back-1])) back--;
                    mlt -= back;

                    if ((mlt >= minMatchLongOff) || ((U32)(ip - match) < LZ5_MAX_16BIT_OFFSET))
                    if (!longest || (mlt > longest && LZ5_better_price(ctx, *startpos, (*startpos - *matchpos), longest, ip, (U32)(ip - match), mlt, ctx->last_off)))
                    { longest = mlt; *startpos = ip+back;  *matchpos = match+back; }   /* virtual matchpos */
                }
            }
        }
        matchIndex -= chainTable[matchIndex & contentMask];
    }

    return longest;
}




FORCE_INLINE int LZ5_compress_lowestPrice(
        LZ5_stream_t* const ctx,
        const BYTE* ip,
        const BYTE* const iend)
{
    const BYTE* anchor = ip;
    const BYTE* const mflimit = iend - MFLIMIT;
    const BYTE* const matchlimit = (iend - LASTLITERALS);

    size_t   ml, ml2, ml0;
    const BYTE* ref=NULL;
    const BYTE* start2=NULL;
    const BYTE* ref2=NULL;
    const BYTE* start0;
    const BYTE* ref0;
    const BYTE* lowPrefixPtr = ctx->base + ctx->dictLimit;
    const size_t minMatchLongOff = ctx->params.minMatchLongOff;
    const size_t sufficient_len = ctx->params.sufficientLength;

    /* Main Loop */
    while (ip < mflimit)
    {
        LZ5_Insert(ctx, ip);
        ml = LZ5_FindMatchLowestPrice (ctx, ip, matchlimit, (&ref));
        if (!ml) { ip++; continue; }

        {
            int back = 0;
            while ((ip + back > anchor) && (ref + back > lowPrefixPtr) && (ip[back - 1] == ref[back - 1])) back--;
            ml -= back;
            ip += back;
            ref += back;
        }

        /* saved, in case we would skip too much */
        start0 = ip;
        ref0 = ref;
        ml0 = ml;
    //    goto _Encode;

_Search:
        if (ip+ml >= mflimit) { goto _Encode; }
        if (ml >= sufficient_len) { goto _Encode; }

        LZ5_Insert(ctx, ip);
        ml2 = (int)LZ5_GetWiderMatch(ctx, ip + ml - 2, anchor, matchlimit, 0, &ref2, &start2);
        if (!ml2) goto _Encode;

        {
        U64 price, best_price;
        int off0=0, off1=0;
        const BYTE *pos, *best_pos;

    //	find the lowest price for encoding ml bytes
        best_pos = ip;
        best_price = LZ5_MAX_PRICE;
        off0 = (int)(ip - ref);
        off1 = (int)(start2 - ref2);

        for (pos = ip + ml; pos >= start2; pos--)
        {
            int common0 = (int)(pos - ip);
            if (common0 >= MINMATCH) {
                price = (int)LZ5_get_price_LZ5v2(ctx, ctx->last_off, ip, ctx->off24pos, ip - anchor, (off0 == ctx->last_off) ? 0 : off0, common0);
                
                {
                    int common1 = (int)(start2 + ml2 - pos);
                    if (common1 >= MINMATCH)
                        price += LZ5_get_price_LZ5v2(ctx, ctx->last_off, pos, ctx->off24pos, 0, (off1 == off0) ? 0 : (off1), common1);
                    else
                        price += LZ5_get_price_LZ5v2(ctx, ctx->last_off, pos, ctx->off24pos, common1, 0, 0);
                }

                if (price < best_price) {
                    best_price = price;
                    best_pos = pos;
                }
            } else {
                price = LZ5_get_price_LZ5v2(ctx, ctx->last_off, ip, ctx->off24pos, start2 - anchor, (off1 == ctx->last_off) ? 0 : off1, ml2);

                if (price < best_price)
                    best_pos = pos;
                break;
            }
        }
    //    LZ5_DEBUG("%u: TRY last_off=%d literals=%u off=%u mlen=%u literals2=%u off2=%u mlen2=%u best=%d\n", (U32)(ip - ctx->inputBuffer), ctx->last_off, (U32)(ip - anchor), off0, (U32)ml,  (U32)(start2 - anchor), off1, ml2, (U32)(best_pos - ip));
        ml = (int)(best_pos - ip);
        }


        if ((ml < MINMATCH) || ((ml < minMatchLongOff) && ((U32)(ip-ref) >= LZ5_MAX_16BIT_OFFSET)))
        {
            ip = start2;
            ref = ref2;
            ml = ml2;
            goto _Search;
        }

_Encode:
        if (start0 < ip)
        {
            if (LZ5_more_profitable(ctx, ip, (ip - ref), ml, start0, (start0 - ref0), ml0, (ref0 - ref), ctx->last_off))
            {
                ip = start0;
                ref = ref0;
                ml = ml0;
            }
        }

     //   if ((ml < minMatchLongOff) && ((U32)(ip-ref) >= LZ5_MAX_16BIT_OFFSET)) { printf("LZ5_encodeSequence ml=%d off=%d\n", ml, (U32)(ip-ref)); exit(0); }
        if (LZ5_encodeSequence_LZ5v2(ctx, &ip, &anchor, ml, ((ip - ref == ctx->last_off) ? ip : ref))) return 0;
    }

    /* Encode Last Literals */
    ip = iend;
    if (LZ5_encodeLastLiterals_LZ5v2(ctx, &ip, &anchor)) goto _output_error;

    /* End */
    return 1;
_output_error:
    return 0;
}

