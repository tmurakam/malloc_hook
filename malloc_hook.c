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

static char static_buffer[256];
static char *buffer_ptr = static_buffer;
static bool initializing = false;
static bool in_hook = false;

static long malloc_total = 0;

typedef struct strMemHeader {
    unsigned long magic;
    size_t size;
} MemHeader;

static const long MAGIC = 0xdeadbeef;

/**
 * Initializer
 */
__attribute__((constructor))
static void ma_init() {
    if (org_malloc == NULL && !initializing) {
        initializing = true;
        org_malloc = dlsym(RTLD_NEXT, "malloc");
        org_realloc = dlsym(RTLD_NEXT, "realloc");
        org_free = dlsym(RTLD_NEXT, "free");
        initializing = false;
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
    ma_init();
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
        // called from dlsym
        ret = buffer_ptr;
        buffer_ptr += size;
        if (buffer_ptr > static_buffer + sizeof(static_buffer)) {
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
        if (header->magic == MAGIC) {
            real_ptr = header;
            oldSize = header->size;
        } else {
            // Not malloced by me...
            hasHeader = false;
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

    pthread_mutex_lock(&ma_mutex);
    if ((char*)ptr < static_buffer || static_buffer + sizeof(static_buffer) <= (char*)ptr) {
        void *real_ptr = ptr;
        MemHeader *header = ptr - sizeof(MemHeader);
        size_t size = 0;
        if (header->magic == MAGIC) {
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
    }
    pthread_mutex_unlock(&ma_mutex);
}

long get_malloc_total() {
    return malloc_total;
}

/**
 * @inherit
 */
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
