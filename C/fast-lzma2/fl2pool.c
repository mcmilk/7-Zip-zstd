/*
 * Copyright (c) 2016-present, Yann Collet, Facebook, Inc.
 * All rights reserved.
 * Modified for FL2 by Conor McCarthy
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */


/* ======   Dependencies   ======= */
#include <stddef.h>  /* size_t */
#include <stdlib.h>  /* malloc, calloc */
#include "fl2pool.h"
#include "fl2_internal.h"

/* ======   Compiler specifics   ====== */
#if defined(_MSC_VER)
#  pragma warning(disable : 4204)        /* disable: C4204: non-constant aggregate initializer */
#endif


#ifndef FL2_SINGLETHREAD

#include "fl2threading.h"   /* pthread adaptation */

/* A job is a function and an opaque argument */
typedef struct FL2POOL_job_s {
    FL2POOL_function function;
    void *opaque;
	size_t n;
} FL2POOL_job;

struct FL2POOL_ctx_s {
    /* Keep track of the threads */
    ZSTD_pthread_t *threads;
    size_t numThreads;

    /* The queue is a single job */
    FL2POOL_job queue;

    /* The number of threads working on jobs */
    size_t numThreadsBusy;
    /* Indicates if the queue is empty */
    int queueEmpty;

    /* The mutex protects the queue */
    ZSTD_pthread_mutex_t queueMutex;
    /* Condition variable for pushers to wait on when the queue is full */
    ZSTD_pthread_cond_t queuePushCond;
    /* Condition variables for poppers to wait on when the queue is empty */
    ZSTD_pthread_cond_t queuePopCond;
    /* Indicates if the queue is shutting down */
    int shutdown;
};

/* FL2POOL_thread() :
   Work thread for the thread pool.
   Waits for jobs and executes them.
   @returns : NULL on failure else non-null.
*/
static void* FL2POOL_thread(void* opaque) {
    FL2POOL_ctx* const ctx = (FL2POOL_ctx*)opaque;
    if (!ctx) { return NULL; }
    for (;;) {
        /* Lock the mutex and wait for a non-empty queue or until shutdown */
        ZSTD_pthread_mutex_lock(&ctx->queueMutex);

        while (ctx->queueEmpty && !ctx->shutdown) {
            ZSTD_pthread_cond_wait(&ctx->queuePopCond, &ctx->queueMutex);
        }
        /* empty => shutting down: so stop */
        if (ctx->queueEmpty) {
            ZSTD_pthread_mutex_unlock(&ctx->queueMutex);
            return opaque;
        }
        /* Pop a job off the queue */
        {   FL2POOL_job const job = ctx->queue;
            ctx->queueEmpty = 1;
            /* Unlock the mutex, signal a pusher, and run the job */
            ZSTD_pthread_mutex_unlock(&ctx->queueMutex);
            ZSTD_pthread_cond_signal(&ctx->queuePushCond);

            job.function(job.opaque, job.n);

			ZSTD_pthread_mutex_lock(&ctx->queueMutex);
			ctx->numThreadsBusy--;
			ZSTD_pthread_mutex_unlock(&ctx->queueMutex);
			ZSTD_pthread_cond_signal(&ctx->queuePushCond);
		}
    }  /* for (;;) */
    /* Unreachable */
}

FL2POOL_ctx* FL2POOL_create(size_t numThreads) {
    FL2POOL_ctx* ctx;
    /* Check the parameters */
    if (!numThreads) { return NULL; }
    /* Allocate the context and zero initialize */
    ctx = (FL2POOL_ctx*)calloc(1, sizeof(FL2POOL_ctx));
    if (!ctx) { return NULL; }
    /* Initialize the job queue.
     * It needs one extra space since one space is wasted to differentiate empty
     * and full queues.
     */
    ctx->numThreadsBusy = 0;
    ctx->queueEmpty = 1;
    (void)ZSTD_pthread_mutex_init(&ctx->queueMutex, NULL);
    (void)ZSTD_pthread_cond_init(&ctx->queuePushCond, NULL);
    (void)ZSTD_pthread_cond_init(&ctx->queuePopCond, NULL);
    ctx->shutdown = 0;
    /* Allocate space for the thread handles */
    ctx->threads = (ZSTD_pthread_t*)malloc(numThreads * sizeof(ZSTD_pthread_t));
    ctx->numThreads = 0;
    /* Check for errors */
    if (!ctx->threads) { FL2POOL_free(ctx); return NULL; }
    /* Initialize the threads */
    {   size_t i;
        for (i = 0; i < numThreads; ++i) {
            if (FL2_pthread_create(&ctx->threads[i], NULL, &FL2POOL_thread, ctx)) {
                ctx->numThreads = i;
                FL2POOL_free(ctx);
                return NULL;
        }   }
        ctx->numThreads = numThreads;
    }
    return ctx;
}

/*! FL2POOL_join() :
    Shutdown the queue, wake any sleeping threads, and join all of the threads.
*/
static void FL2POOL_join(FL2POOL_ctx* ctx) {
    /* Shut down the queue */
    ZSTD_pthread_mutex_lock(&ctx->queueMutex);
    ctx->shutdown = 1;
    ZSTD_pthread_mutex_unlock(&ctx->queueMutex);
    /* Wake up sleeping threads */
    ZSTD_pthread_cond_broadcast(&ctx->queuePushCond);
    ZSTD_pthread_cond_broadcast(&ctx->queuePopCond);
    /* Join all of the threads */
    {   size_t i;
        for (i = 0; i < ctx->numThreads; ++i) {
            FL2_pthread_join(ctx->threads[i], NULL);
    }   }
}

void FL2POOL_free(FL2POOL_ctx *ctx) {
    if (!ctx) { return; }
    FL2POOL_join(ctx);
    ZSTD_pthread_mutex_destroy(&ctx->queueMutex);
    ZSTD_pthread_cond_destroy(&ctx->queuePushCond);
    ZSTD_pthread_cond_destroy(&ctx->queuePopCond);
    free(ctx->threads);
    free(ctx);
}

size_t FL2POOL_sizeof(FL2POOL_ctx *ctx) {
    if (ctx==NULL) return 0;  /* supports sizeof NULL */
    return sizeof(*ctx)
        + ctx->numThreads * sizeof(ZSTD_pthread_t);
}

void FL2POOL_add(void* ctxVoid, FL2POOL_function function, void *opaque, size_t n) {
    FL2POOL_ctx* const ctx = (FL2POOL_ctx*)ctxVoid;
    if (!ctx)
		return; 

    ZSTD_pthread_mutex_lock(&ctx->queueMutex);
    {   FL2POOL_job const job = {function, opaque, n};

        /* Wait until there is space in the queue for the new job */
        while (!ctx->queueEmpty && !ctx->shutdown) {
          ZSTD_pthread_cond_wait(&ctx->queuePushCond, &ctx->queueMutex);
        }
        /* The queue is still going => there is space */
        if (!ctx->shutdown) {
			ctx->numThreadsBusy++;
			ctx->queueEmpty = 0;
            ctx->queue = job;
        }
    }
    ZSTD_pthread_mutex_unlock(&ctx->queueMutex);
    ZSTD_pthread_cond_signal(&ctx->queuePopCond);
}

void FL2POOL_waitAll(void *ctxVoid)
{
    FL2POOL_ctx* const ctx = (FL2POOL_ctx*)ctxVoid;
    if (!ctx) { return; }

    ZSTD_pthread_mutex_lock(&ctx->queueMutex);
    while (ctx->numThreadsBusy && !ctx->shutdown) {
        ZSTD_pthread_cond_wait(&ctx->queuePushCond, &ctx->queueMutex);
    }
    ZSTD_pthread_mutex_unlock(&ctx->queueMutex);
}

#endif  /* FL2_SINGLETHREAD */
