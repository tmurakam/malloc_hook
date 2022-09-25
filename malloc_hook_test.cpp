#include <gtest/gtest.h>

#include "malloc_hook.h"

static void *last_malloc_ptr;
static int last_malloc_size;

void malloc_hook(void *ptr, size_t size, void *caller) {
    printf("malloc: ptr=%p, size=%ld, caller=%p\n", ptr, size, caller);
}

void free_hook(void *ptr, void *caller) {
    printf("free: ptr=%p, caller=%p\n", ptr, caller);
}

TEST(MallocHookTest, init) {
    set_malloc_hook(malloc_hook);
    set_free_hook(free_hook);

    void *p = malloc(100);
    free(p);
}