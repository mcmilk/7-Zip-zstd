// sha1.h
// This file from UnRar sources

#ifndef _RAR_SHA1_
#define _RAR_SHA1_

#define HW 5

typedef struct {
    UINT32 state[5];
    UINT32 count[2];
    unsigned char buffer[64];
} hash_context;

void hash_initial( hash_context * c );
void hash_process( hash_context * c, unsigned char * data, unsigned len );
void hash_final( hash_context * c, UINT32[HW] );

#endif
