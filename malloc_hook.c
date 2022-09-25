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

static void (*malloc_hook)(void *ptr, size_t size, void *caller) = NULL;
static void (*realloc_hook)(void *ptr, void *newptr, size_t size, void *caller) = NULL;
static void (*free_hook)(void *ptr, void *caller) = NULL;

static void * (*org_malloc)(size_t) = NULL;
static void * (*org_realloc)(void *, size_t) = NULL;
static void (*org_free)(void *) = NULL;

static char static_buffer[256];
static char *buffer_ptr = static_buffer;
static bool ma_initializing = false;
static bool hooking = false;

/**
 * Initializer
 */
__attribute__((constructor))
static void ma_init() {
    if (org_malloc == NULL && !ma_initializing) {
        ma_initializing = true;
        org_malloc = dlsym(RTLD_NEXT, "malloc");
        org_realloc = dlsym(RTLD_NEXT, "realloc");
        org_free = dlsym(RTLD_NEXT, "free");
        ma_initializing = false;
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
void set_malloc_hook(void (*hook)(void *ptr, size_t size, void *caller)) {
    pthread_mutex_lock(&ma_mutex);
    malloc_hook = hook;
    pthread_mutex_unlock(&ma_mutex);
}

/**
 * realloc hook
 * @param size
 * @param caller
 */
void set_realloc_hook(void (*hook)(void *ptr, void *newptr, size_t size, void *caller)) {
    pthread_mutex_lock(&ma_mutex);
    realloc_hook = hook;
    pthread_mutex_unlock(&ma_mutex);
}

/**
 * free hook
 * @param ptr
 * @param caller
 */
void set_free_hook(void (*hook)(void *ptr, void *caller)) {
    pthread_mutex_lock(&ma_mutex);
    free_hook = hook;
    pthread_mutex_unlock(&ma_mutex);
}

void *malloc(size_t size) {
    void *ret;

    pthread_mutex_lock(&ma_mutex);
    ma_init();
    if (org_malloc != NULL) {
        ret = org_malloc(size);
        if (malloc_hook && !hooking) {
            hooking = true;
            malloc_hook(ret, size, __builtin_return_address(0));
            hooking = false;
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

void *calloc(size_t n, size_t size) {
    void *ptr = malloc(n * size);
    memset(ptr, 0, n * size);
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    pthread_mutex_lock(&ma_mutex);
    void *ret = org_realloc(ptr, size);
    if (realloc_hook && !hooking) {
        hooking = true;
        realloc_hook(ptr, ret, size, __builtin_return_address(0));
        hooking = false;
    }
    pthread_mutex_unlock(&ma_mutex);
    return ret;
}

void free(void *ptr) {
    pthread_mutex_lock(&ma_mutex);
    if ((char*)ptr < static_buffer || static_buffer + sizeof(static_buffer) <= (char*)ptr) {
        if (free_hook && !hooking) {
            hooking = true;
            free_hook(ptr, __builtin_return_address(0));
            hooking = false;
        }
        org_free(ptr);
    }
    pthread_mutex_unlock(&ma_mutex);
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
