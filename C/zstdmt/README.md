
# Multithreading Library for [LZ4], [LZ5] and [ZStandard]


### Compression
```
typedef struct {
	void *buf;		/* ptr to data */
	size_t size;		/* current filled in buf */
	size_t allocated;	/* length of buf */
} LZ4MT_Buffer;

/**
 * reading and writing functions
 * - you can use stdio functions or plain read/write ...
 * - a sample is given in 7-Zip ZS or lz4mt.c
 */
typedef int (fn_read) (void *args, LZ4MT_Buffer * in);
typedef int (fn_write) (void *args, LZ4MT_Buffer * out);

typedef struct {
	fn_read *fn_read;
	void *arg_read;
	fn_write *fn_write;
	void *arg_write;
} LZ4MT_RdWr_t;

typedef struct LZ4MT_CCtx_s LZ4MT_CCtx;

/* 1) allocate new cctx */
LZ4MT_CCtx *LZ4MT_createCCtx(int threads, int level, int inputsize);

/* 2) threaded compression */
size_t LZ4MT_CompressCCtx(LZ4MT_CCtx * ctx, LZ4MT_RdWr_t * rdwr);

/* 3) get some statistic */
size_t LZ4MT_GetFramesCCtx(LZ4MT_CCtx * ctx);
size_t LZ4MT_GetInsizeCCtx(LZ4MT_CCtx * ctx);
size_t LZ4MT_GetOutsizeCCtx(LZ4MT_CCtx * ctx);

/* 4) free cctx */
void LZ4MT_freeCCtx(LZ4MT_CCtx * ctx);
```

### Decompression
```
typedef struct LZ4MT_DCtx_s LZ4MT_DCtx;

/* 1) allocate new cctx */
LZ4MT_DCtx *LZ4MT_createDCtx(int threads, int inputsize);

/* 2) threaded compression */
size_t LZ4MT_DecompressDCtx(LZ4MT_DCtx * ctx, LZ4MT_RdWr_t * rdwr);

/* 3) get some statistic */
size_t LZ4MT_GetFramesDCtx(LZ4MT_DCtx * ctx);
size_t LZ4MT_GetInsizeDCtx(LZ4MT_DCtx * ctx);
size_t LZ4MT_GetOutsizeDCtx(LZ4MT_DCtx * ctx);

/* 4) free cctx */
void LZ4MT_freeDCtx(LZ4MT_DCtx * ctx);
```

## Todo

- add Makefile
