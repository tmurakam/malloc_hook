/*
 * Memory Hook library for debugging
 * https://github.com/tmurakam/malloc_hook
 *
 * Copyright (c) 2022, Takuya Murakami
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <pthread.h>
#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>
#include <execinfo.h>
#include <stdio.h>

#include "malloc_hook.h"

static pthread_mutex_t ma_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static malloc_hook_t malloc_hook = NULL;
static realloc_hook_t realloc_hook = NULL;
static free_hook_t free_hook = NULL;

static void * (*org_malloc)(size_t) = NULL;
static void * (*org_realloc)(void *, size_t) = NULL;
static void (*org_free)(void *) = NULL;

static bool initializing = false;
static bool in_hook = false;

// initial memory area used by malloc inside dlsym.
static char static_buffer[256];
static char *buffer_ptr = static_buffer;

static long malloc_total = 0;

/**
 * Memory header
 */
typedef struct strMemHeader {
    unsigned long magic;  // Magic
    size_t size;  // allocated memory size (excludes this header)
} MemHeader;

/** MAGIC number of header */
static const long MAGIC = 0xdeadbeef;

/**
 * Initializer
 */
__attribute__((constructor))
static void ma_init() {
    if (org_malloc == NULL && !initializing) {
        pthread_mutex_lock(&ma_mutex);
        initializing = true; // recursive guard
        org_malloc = dlsym(RTLD_NEXT, "malloc");
        org_realloc = dlsym(RTLD_NEXT, "realloc");
        org_free = dlsym(RTLD_NEXT, "free");
        initializing = false;
        pthread_mutex_unlock(&ma_mutex);
    }
}

/**
 * De-initializer
 */
__attribute__((destructor))
static void ma_exit() {
}

/**
 * malloc hook
 * @param size
 * @param caller
 */
void set_malloc_hook(malloc_hook_t hook) {
    pthread_mutex_lock(&ma_mutex);
    malloc_hook = hook;
    pthread_mutex_unlock(&ma_mutex);
}

/**
 * realloc hook
 * @param size
 * @param caller
 */
void set_realloc_hook(realloc_hook_t hook) {
    pthread_mutex_lock(&ma_mutex);
    realloc_hook = hook;
    pthread_mutex_unlock(&ma_mutex);
}

/**
 * free hook
 * @param ptr
 * @param caller
 */
void set_free_hook(free_hook_t hook) {
    pthread_mutex_lock(&ma_mutex);
    free_hook = hook;
    pthread_mutex_unlock(&ma_mutex);
}

/**
 * replaced malloc
 */
void *malloc(size_t size) {
    void *ret;

    pthread_mutex_lock(&ma_mutex);

    ma_init();  // note: this may call malloc recursively

    if (org_malloc != NULL) {
        MemHeader *header = org_malloc(sizeof(MemHeader) + size);
        if (header) {
            malloc_total += size;
            header->magic = MAGIC;
            header->size = size;
            ret = header + 1;
            if (malloc_hook && !in_hook) {
                in_hook = true;
                malloc_hook(ret, size, __builtin_return_address(0));
                in_hook = false;
            }
        }
    } else {
        // called from initial dlsym
        ret = buffer_ptr;
        buffer_ptr += size;
        if (buffer_ptr > static_buffer + sizeof(static_buffer)) {
            // oops, no memory.
            exit(2);
        }
    }
    pthread_mutex_unlock(&ma_mutex);
    return ret;
}

/**
 * replaced calloc
 */
void *calloc(size_t n, size_t size) {
    void *ptr = malloc(n * size);
    memset(ptr, 0, n * size);
    return ptr;
}

static bool checkHeader(MemHeader *header) {
    if (header->magic == MAGIC) {
        return true;
    } else {
        // Never happen, not managed by me...
        fprintf(stderr, "WARNING: memory header broken: %p\n", header);
        return false;
    }
}

/**
 * replaced realloc
 */
void *realloc(void *oldPtr, size_t newSize) {
    pthread_mutex_lock(&ma_mutex);

    bool hasHeader = true;
    MemHeader *header = NULL;
    size_t oldSize = 0;
    void *real_ptr = oldPtr;

    if (oldPtr != NULL) {
        header = oldPtr - sizeof(MemHeader);
        hasHeader = checkHeader(header);
        if (hasHeader) {
            real_ptr = header;
            oldSize = header->size;
        }
    }

    void *newPtr = org_realloc(real_ptr, hasHeader ? newSize + sizeof(MemHeader) : newSize);
    if (newPtr) {
        void *newRealPtr = newPtr;
        if (hasHeader) {
            header = newRealPtr;
            header->magic = MAGIC;
            header->size = newSize;
            newPtr = header + 1;
        }
        malloc_total += newSize - oldSize;

        if (realloc_hook && !in_hook) {
            in_hook = true;
            realloc_hook(oldPtr, oldSize, newPtr, newSize, __builtin_return_address(0));
            in_hook = false;
        }
    }
    pthread_mutex_unlock(&ma_mutex);
    return newPtr;
}

/**
 * replaced free
 */
void free(void *ptr) {
    if (!ptr) return;

    if (static_buffer <= (char*)ptr && (char *)ptr < static_buffer + sizeof(static_buffer)) {
        // initial malloc area, do nothing
        return;
    }

    pthread_mutex_lock(&ma_mutex);
    void *real_ptr = ptr;
    MemHeader *header = ptr - sizeof(MemHeader);
    size_t size = 0;
    if (checkHeader(header)) {
        real_ptr = header;
        size = header->size;
    }
    malloc_total -= size;

    if (free_hook && !in_hook) {
        in_hook = true;
        free_hook(ptr, size, __builtin_return_address(0));
        in_hook = false;
    }
    org_free(real_ptr);
    pthread_mutex_unlock(&ma_mutex);
}

long get_malloc_total() {
    return malloc_total;
}

void dump_backtrace(int depth) {
    void *trace[depth];
    int n = backtrace(trace, depth);
    char **symbols = backtrace_symbols(trace, n);

    fprintf(stderr, "backtrace:\n");
    for (int i = 0; i < n; i++) {
        fprintf(stderr, "  [%d] %s\n", i, symbols[i]);
    }
    free(symbols);
}
