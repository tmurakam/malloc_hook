#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*malloc_hook_t)(void *ptr, size_t size, void *caller);
typedef void (*realloc_hook_t)(void *ptr, void *newptr, size_t size, void *caller);
typedef void (*free_hook_t)(void *ptr, void *caller);

/**
 * set malloc hook
 * @param hook Hook
 */
void set_malloc_hook(malloc_hook_t hook);

/**
 * set realloc hook
 * @param hook Hook
 */
void set_realloc_hook(realloc_hook_t hook);

/**
 * set free hook
 * @param hook Hook
 */
void set_free_hook(free_hook_t hook);

/**
 * dump backtrace
 * @param depth Max stack dump depth
 */
void dump_backtrace(int depth);

#ifdef __cplusplus
};
#endif
