/*
 * Memory Hook library for debugging
 * https://github.com/tmurakam/malloc_hook
 *
 * Copyright (c) 2022, Takuya Murakami.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, 
 *      this list of conditions and the following disclaimer.
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
#include <assert.h>

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
static bool _in_backtrace = false;

/**
 * Memory header
 */
typedef struct strMemHeader {
    unsigned long magic;  // Magic
    struct strMemHeader *prev;
    struct strMemHeader *next;
    size_t size;  // allocated memory size (excludes this header)
    void *caller[MALLOC_MAX_BACKTRACE];
} MemHeader;

/** MAGIC number of header */
static const long MAGIC = 0xdeadbeef;

static MemHeader *header_head = NULL;
static MemHeader *header_tail = NULL;

/** dump mark: latest entry which NOT be shown */
static MemHeader *dump_mark = NULL;

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

void insert_header(MemHeader *header) {
    if (header_head == NULL) {
        header->prev = header->next = NULL;
        header_head = header_tail = header;
    } else {
        header_tail->next = header;
        header->prev = header_tail;
        header->next = NULL;
        header_tail = header;
    }
}

void remove_header(MemHeader *header) {
    if (header == dump_mark) {
        dump_mark = header->prev;
    }

    if (header->prev != NULL) {
        header->prev->next = header->next;
    } else {
        header_head = header->next;
    }
    if (header->next != NULL) {
        header->next->prev = header->prev;
    } else {
        header_tail = header->prev;
    }
}

__attribute__((noinline))
static void** get_backtrace(void **trace) {
    const int skip = 2; // this + caller = 2
    void *_trace[MALLOC_MAX_BACKTRACE + skip];
    memset(_trace, 0, sizeof(_trace));
    if (!_in_backtrace) { // guard malloc in backtrace
        _in_backtrace = true;
        int n = backtrace(_trace, MALLOC_MAX_BACKTRACE + skip);
        _in_backtrace = false;
        memcpy(trace, _trace + skip, sizeof(void*) * MALLOC_MAX_BACKTRACE);
    } else {
        memset(trace, 0, sizeof(void *) * MALLOC_MAX_BACKTRACE);
        trace[0] = __builtin_return_address(1);
    }
    return trace;
}

static void *malloc_sub(size_t size, void **callers) {
    void *ret;

    pthread_mutex_lock(&ma_mutex);

    ma_init();  // note: this may call malloc recursively

    if (org_malloc != NULL) {
        MemHeader *header = org_malloc(sizeof(MemHeader) + size);
        if (header) {
            malloc_total += size;
            header->magic = MAGIC;
            header->size = size;
            memcpy(header->caller, callers, sizeof(void*) * MALLOC_MAX_BACKTRACE);
            ret = header + 1;
            insert_header(header);

            if (malloc_hook && !in_hook) {
                in_hook = true;
                malloc_hook(ret, size, header->caller);
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
 * replaced malloc
 */
void *malloc(size_t size) {
    void *callers[MALLOC_MAX_BACKTRACE];
    get_backtrace(callers);
    //assert(callers[0] == __builtin_return_address(0));

    return malloc_sub(size, callers);
}

/**
 * replaced calloc
 */
void *calloc(size_t n, size_t size) {
    void *callers[MALLOC_MAX_BACKTRACE];
    get_backtrace(callers);
    //assert(callers[0] == __builtin_return_address(0));

    void *ptr = malloc_sub(n * size, callers);
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
    if (newSize == 0) {
	free(oldPtr);
	return NULL;
    }

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
            remove_header(header);
        }
    }

    void *newPtr = org_realloc(real_ptr, hasHeader ? newSize + sizeof(MemHeader) : newSize);
    if (newPtr) {
        void *newRealPtr = newPtr;
        if (hasHeader) {
            header = newRealPtr;
            header->magic = MAGIC;
            header->size = newSize;
            get_backtrace(header->caller);
            //assert(header->caller[0] == __builtin_return_address(0));
            newPtr = header + 1;
            insert_header(header);
        }
        malloc_total += newSize - oldSize;

        if (realloc_hook && !in_hook) {
            in_hook = true;
            realloc_hook(oldPtr, oldSize, newPtr, newSize, hasHeader ? header->caller : NULL);
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
        remove_header(header);
    }
    malloc_total -= size;

    if (free_hook && !in_hook) {
        in_hook = true;
        free_hook(ptr, size, header->caller);
        in_hook = false;
    }
    org_free(real_ptr);
    pthread_mutex_unlock(&ma_mutex);
}

long get_malloc_total() {
    return malloc_total;
}

void malloc_heap_dump_mark() {
    pthread_mutex_lock(&ma_mutex);
    dump_mark = header_tail;
    pthread_mutex_unlock(&ma_mutex);
}

void malloc_heap_dump_unmark() {
    pthread_mutex_lock(&ma_mutex);
    dump_mark = NULL;
    pthread_mutex_unlock(&ma_mutex);
}

void malloc_heap_dump(FILE *fp, bool resolve_symbol) {
    char symbol[1024];
    size_t total = 0;
    pthread_mutex_lock(&ma_mutex);
    fprintf(fp, "== Start heap dump\n");

    int i = 0;
    for (MemHeader *header = header_tail; header; header = header->prev, i++) { // tail to head
        if (header == dump_mark) {
            break;
        }
        if (header->magic != MAGIC) {
            fprintf(fp, "WARNING: bad header magic [%p], abort dump.", header + 1);
            break;
        }
        total += header->size;

        fprintf(fp, "%d: [%p] size=%ld\n", i, header + 1, header->size);
        in_hook = true; // don't call hook in this function
        for (int j = 0; j < MALLOC_MAX_BACKTRACE; j++) {
            void *caller = header->caller[j];
            if (!caller) break;
            if (resolve_symbol) {
                get_caller_symbol(caller, symbol, sizeof(symbol));
                fprintf(fp, "  - %s\n", symbol);
            } else {
                fprintf(fp, "  - %p\n", caller);
            }
        }
        in_hook = false;
    }

    fprintf(fp, "== End heap dump: Total heap usage = %ld\n", total);
    pthread_mutex_unlock(&ma_mutex);
}

void get_caller_symbol(void *caller, char *buffer, int buflen) {
    char **symbols = backtrace_symbols(&caller, 1);
    strncpy(buffer, symbols[0], buflen);
    free(symbols);
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
