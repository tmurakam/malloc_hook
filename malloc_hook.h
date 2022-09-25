#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * malloc hook
 * @param ptr
 * @param size
 * @param caller
 */
void set_malloc_hook(void (*hook)(void *ptr, size_t size, void *caller));

/**
 * realloc hook
 * @param ptr
 * @param newptr
 * @param size
 * @param caller
 */
void set_realloc_hook(void (*hook)(void *ptr, void *newptr, size_t size, void *caller));

/**
 * free hook
 * @param ptr
 * @param caller
 */
void set_free_hook(void (*hook)(void *ptr, void *caller));

/**
 * dump backtrace
 * @param depth
 */
void dump_backtrace(int depth);

#ifdef __cplusplus
};
#endif
