#define LIZ_FREQ_DIV   5

FORCE_INLINE void LIZ_setLog2Prices(LIZ_stream_t* ctx)
{
    ctx->log2LitSum = LIZ_highbit32(ctx->litSum+1);
    ctx->log2FlagSum = LIZ_highbit32(ctx->flagSum+1);
}


MEM_STATIC void LIZ_rescaleFreqs(LIZ_stream_t* ctx)
{
    unsigned u;

    ctx->cachedLiterals = NULL;
    ctx->cachedPrice = ctx->cachedLitLength = 0;
    
    ctx->litPriceSum = 0;

    if (ctx->litSum == 0) {
        ctx->litSum = 2 * 256;
        ctx->flagSum = 2 * 256;

        for (u=0; u < 256; u++) {
            ctx->litFreq[u] = 2;
            ctx->flagFreq[u] = 2;
        }
    } else {
        ctx->litSum = 0;
        ctx->flagSum = 0;

        for (u=0; u < 256; u++) {
            ctx->litFreq[u] = 1 + (ctx->litFreq[u]>>LIZ_FREQ_DIV);
            ctx->litSum += ctx->litFreq[u];
            ctx->flagFreq[u] = 1 + (ctx->flagFreq[u]>>LIZ_FREQ_DIV);
            ctx->flagSum += ctx->flagFreq[u];
        }
    }

    LIZ_setLog2Prices(ctx);
}


FORCE_INLINE int LIZ_encodeSequence_LZ5v2 (
    LIZ_stream_t* ctx,
    const BYTE** ip,
    const BYTE** anchor,
    size_t matchLength,
    const BYTE* const match)
{
    U32 offset = (U32)(*ip - match);
    size_t length = (size_t)(*ip - *anchor);
    BYTE* token = (ctx->flagsPtr)++;

    if (length > 0 || offset < LIZ_MAX_16BIT_OFFSET) {
        /* Encode Literal length */
      //  if ((limitedOutputBuffer) && (ctx->literalsPtr > oend - length - LIZ_LENGTH_SIZE_LZ5v2(length) - WILDCOPYLENGTH)) { LIZ_LOG_COMPRESS_LZ5v2("encodeSequence overflow1\n"); return 1; }   /* Check output limit */
        if (length >= MAX_SHORT_LITLEN) 
        {   size_t len; 
            *token = MAX_SHORT_LITLEN; 
            len = length - MAX_SHORT_LITLEN;
            if (len >= (1<<16)) { *(ctx->literalsPtr) = 255;  MEM_writeLE24(ctx->literalsPtr+1, (U32)(len));  ctx->literalsPtr += 4; }
            else if (len >= 254) { *(ctx->literalsPtr) = 254;  MEM_writeLE16(ctx->literalsPtr+1, (U16)(len));  ctx->literalsPtr += 3; }
            else *(ctx->literalsPtr)++ = (BYTE)len;
        }
        else *token = (BYTE)length;

        /* Copy Literals */
        LIZ_wildCopy(ctx->literalsPtr, *anchor, (ctx->literalsPtr) + length);
#ifndef LIZ_NO_HUFFMAN
        if (ctx->huffType) { 
            ctx->litSum += (U32)length;
            ctx->litPriceSum += (U32)(length * ctx->log2LitSum);
            {   U32 u;
                for (u=0; u < length; u++) {
                    ctx->litPriceSum -= LIZ_highbit32(ctx->litFreq[ctx->literalsPtr[u]]+1);
                    ctx->litFreq[ctx->literalsPtr[u]]++;
            }   }
        }
#endif
        ctx->literalsPtr += length;


        if (offset >= LIZ_MAX_16BIT_OFFSET) {
            COMPLOG_CODEWORDS_LZ5v2("T32+ literal=%u match=%u offset=%d\n", (U32)length, 0, 0);
            *token+=(1<<ML_RUN_BITS);
#ifndef LIZ_NO_HUFFMAN
            if (ctx->huffType) { 
                ctx->flagFreq[*token]++;
                ctx->flagSum++;
            }
#endif
            token = (ctx->flagsPtr)++;
        }
    }

    /* Encode Offset */
    if (offset >= LIZ_MAX_16BIT_OFFSET)  // 24-bit offset
    {
        if (matchLength < MM_LONGOFF) printf("ERROR matchLength=%d/%d\n", (int)matchLength, MM_LONGOFF), exit(0);

      //  if ((limitedOutputBuffer) && (ctx->literalsPtr > oend - 8 /*LIZ_LENGTH_SIZE_LZ5v2(length)*/)) { LIZ_LOG_COMPRESS_LZ5v2("encodeSequence overflow2\n"); return 1; }   /* Check output limit */
        if (matchLength - MM_LONGOFF >= LIZ_LAST_LONG_OFF) 
        {
            size_t len = matchLength - MM_LONGOFF - LIZ_LAST_LONG_OFF;
            *token = LIZ_LAST_LONG_OFF;
            if (len >= (1<<16)) { *(ctx->literalsPtr) = 255;  MEM_writeLE24(ctx->literalsPtr+1, (U32)(len));  ctx->literalsPtr += 4; }
            else if (len >= 254) { *(ctx->literalsPtr) = 254;  MEM_writeLE16(ctx->literalsPtr+1, (U16)(len));  ctx->literalsPtr += 3; }
            else *(ctx->literalsPtr)++ = (BYTE)len; 
            COMPLOG_CODEWORDS_LZ5v2("T31 literal=%u match=%u offset=%d\n", 0, (U32)matchLength, offset);
        }
        else
        {
            COMPLOG_CODEWORDS_LZ5v2("T0-30 literal=%u match=%u offset=%d\n", 0, (U32)matchLength, offset);
            *token = (BYTE)(matchLength - MM_LONGOFF);
        }

        MEM_writeLE24(ctx->offset24Ptr, offset); 
        ctx->offset24Ptr += 3;
        ctx->last_off = offset;
        ctx->off24pos = *ip;
    }
    else
    {
        COMPLOG_CODEWORDS_LZ5v2("T32+ literal=%u match=%u offset=%d\n", (U32)length, (U32)matchLength, offset);
        if (offset == 0)
        {
            *token+=(1<<ML_RUN_BITS);
        }
        else
        {
            if (offset < 8) printf("ERROR offset=%d\n", (int)offset);
            if (matchLength < MINMATCH) { printf("matchLength[%d] < MINMATCH  offset=%d\n", (int)matchLength, (int)ctx->last_off); exit(1); }
            
            ctx->last_off = offset;
            MEM_writeLE16(ctx->offset16Ptr, (U16)ctx->last_off); ctx->offset16Ptr += 2;
        }

        /* Encode MatchLength */
        length = matchLength;
       // if ((limitedOutputBuffer) && (ctx->literalsPtr > oend - 5 /*LIZ_LENGTH_SIZE_LZ5v2(length)*/)) { LIZ_LOG_COMPRESS_LZ5v2("encodeSequence overflow2\n"); return 1; }   /* Check output limit */
        if (length >= MAX_SHORT_MATCHLEN) {
            *token += (BYTE)(MAX_SHORT_MATCHLEN<<RUN_BITS_LZ5v2);
            length -= MAX_SHORT_MATCHLEN;
            if (length >= (1<<16)) { *(ctx->literalsPtr) = 255;  MEM_writeLE24(ctx->literalsPtr+1, (U32)(length));  ctx->literalsPtr += 4; }
            else if (length >= 254) { *(ctx->literalsPtr) = 254;  MEM_writeLE16(ctx->literalsPtr+1, (U16)(length));  ctx->literalsPtr += 3; }
            else *(ctx->literalsPtr)++ = (BYTE)length;
        }
        else *token += (BYTE)(length<<RUN_BITS_LZ5v2);
    }

#ifndef LIZ_NO_HUFFMAN
    if (ctx->huffType) { 
        ctx->flagFreq[*token]++;
        ctx->flagSum++;
        LIZ_setLog2Prices(ctx);
    }
#endif

    /* Prepare next loop */
    *ip += matchLength;
    *anchor = *ip;

    return 0;
}


FORCE_INLINE int LIZ_encodeLastLiterals_LZ5v2 (
    LIZ_stream_t* ctx,
    const BYTE** ip,
    const BYTE** anchor)
{
    size_t length = (int)(*ip - *anchor);
    (void)ctx;

    memcpy(ctx->literalsPtr, *anchor, length);
    ctx->literalsPtr += length;
    return 0;
}


#define LIZ_PRICE_MULT 1
#define LIZ_GET_TOKEN_PRICE_LZ5v2(token)  (LIZ_PRICE_MULT * (ctx->log2FlagSum - LIZ_highbit32(ctx->flagFreq[token]+1)))


FORCE_INLINE size_t LIZ_get_price_LZ5v2(LIZ_stream_t* const ctx, int rep, const BYTE *ip, const BYTE *off24pos, size_t litLength, U32 offset, size_t matchLength) 
{
    size_t price = 0;
    BYTE token = 0;
#ifndef LIZ_NO_HUFFMAN
    const BYTE* literals = ip - litLength;
    U32 u;
    if ((ctx->huffType) && (ctx->params.parserType != LIZ_parser_lowestPrice)) {
        if (ctx->cachedLiterals == literals && litLength >= ctx->cachedLitLength) {
            size_t const additional = litLength - ctx->cachedLitLength;
        //    printf("%d ", (int)litLength - (int)ctx->cachedLitLength);
            const BYTE* literals2 = ctx->cachedLiterals + ctx->cachedLitLength;
            price = ctx->cachedPrice + LIZ_PRICE_MULT * additional * ctx->log2LitSum;
            for (u=0; u < additional; u++)
                price -= LIZ_PRICE_MULT * LIZ_highbit32(ctx->litFreq[literals2[u]]+1);
            ctx->cachedPrice = (U32)price;
            ctx->cachedLitLength = (U32)litLength;
        } else {
            price = LIZ_PRICE_MULT * litLength * ctx->log2LitSum;
            for (u=0; u < litLength; u++)
                price -= LIZ_PRICE_MULT * LIZ_highbit32(ctx->litFreq[literals[u]]+1);

            if (litLength >= 12) {
                ctx->cachedLiterals = literals;
                ctx->cachedPrice = (U32)price;
                ctx->cachedLitLength = (U32)litLength;
            }
        }
    }
    else
        price += 8*litLength;  /* Copy Literals */
#else
    price += 8*litLength;  /* Copy Literals */
    (void)ip;
    (void)ctx;
#endif

    (void)off24pos;
    (void)rep;

    if (litLength > 0 || offset < LIZ_MAX_16BIT_OFFSET) {
        /* Encode Literal length */
        if (litLength >= MAX_SHORT_LITLEN) 
        {   size_t len = litLength - MAX_SHORT_LITLEN;
            token = MAX_SHORT_LITLEN; 
            if (len >= (1<<16)) price += 32;
            else if (len >= 254) price += 24;
            else price += 8;
        }
        else token = (BYTE)litLength;

        if (offset >= LIZ_MAX_16BIT_OFFSET) {
            token+=(1<<ML_RUN_BITS);
            if (ctx->huffType && ctx->params.parserType != LIZ_parser_lowestPrice)
                price += LIZ_GET_TOKEN_PRICE_LZ5v2(token);
            else
                price += 8;
       }
    }

    /* Encode Offset */
    if (offset >= LIZ_MAX_16BIT_OFFSET) { // 24-bit offset
        if (matchLength < MM_LONGOFF) return LIZ_MAX_PRICE; // error

        if (matchLength - MM_LONGOFF >= LIZ_LAST_LONG_OFF) {
            size_t len = matchLength - MM_LONGOFF - LIZ_LAST_LONG_OFF;
            token = LIZ_LAST_LONG_OFF;
            if (len >= (1<<16)) price += 32;
            else if (len >= 254) price += 24;
            else price += 8;
        } else {
            token = (BYTE)(matchLength - MM_LONGOFF);
        }

        price += 24;
    } else {
        size_t length;
        if (offset == 0) {
            token+=(1<<ML_RUN_BITS);
        } else {
            if (offset < 8) return LIZ_MAX_PRICE; // error
            if (matchLength < MINMATCH) return LIZ_MAX_PRICE; // error
            price += 16;
        }

        /* Encode MatchLength */
        length = matchLength;
        if (length >= MAX_SHORT_MATCHLEN) {
            token += (BYTE)(MAX_SHORT_MATCHLEN<<RUN_BITS_LZ5v2);
            length -= MAX_SHORT_MATCHLEN;
            if (length >= (1<<16)) price += 32;
            else if (length >= 254) price += 24;
            else price += 8;
        }
        else token += (BYTE)(length<<RUN_BITS_LZ5v2);
    }

    if (offset > 0 || matchLength > 0) {
        int offset_load = LIZ_highbit32(offset);
        if (ctx->huffType) {
            price += ((offset_load>=20) ? ((offset_load-19)*4) : 0);
            price += 4 + (matchLength==1);
        } else {
            price += ((offset_load>=16) ? ((offset_load-15)*4) : 0);
            price += 6 + (matchLength==1);
        }
        if (ctx->huffType && ctx->params.parserType != LIZ_parser_lowestPrice)
            price += LIZ_GET_TOKEN_PRICE_LZ5v2(token);
        else
            price += 8;
    } else {
        if (ctx->huffType && ctx->params.parserType != LIZ_parser_lowestPrice)
            price += LIZ_GET_TOKEN_PRICE_LZ5v2(token);  // 1=better ratio
    }

    return price;
}
