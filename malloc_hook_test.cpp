#include <gtest/gtest.h>

#include "malloc_hook.h"

void malloc_hook(size_t size, void *caller) {
    printf("malloc: size=%d, caller=%x\n", size, caller);
}

void free_hook(void *ptr, void *caller) {
    printf("free: ptr=%x, caller=%x\n", ptr, caller);
}

TEST(MallocHookTest, init) {
    set_malloc_hook(malloc_hook);
    set_free_hook(free_hook);

    void *p = malloc(100);
    free(p);
}