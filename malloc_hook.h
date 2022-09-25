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
 * free hook
 * @param ptr
 * @param caller
 */
void set_free_hook(void (*hook)(void *ptr, void *caller));

#ifdef __cplusplus
};
#endif
