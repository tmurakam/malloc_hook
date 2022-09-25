#include <gtest/gtest.h>
#include <dlfcn.h>

#include "malloc_hook.h"

static void *last_malloc_ptr;
static int last_malloc_size;

void malloc_hook(void *ptr, size_t size, void *caller) {
    fprintf(stderr, "malloc: ptr=%p, size=%ld, caller=%p\n", ptr, size, caller);
    last_malloc_ptr = ptr;
    last_malloc_size = size;

    dump_backtrace(15);
}

void realloc_hook(void *ptr, void *newptr, size_t size, void *caller) {
    fprintf(stderr, "realloc: ptr=%p, newptr=%p, size=%ld, caller=%p\n", ptr, newptr, size, caller);

    dump_backtrace(15);
}

void free_hook(void *ptr, void *caller) {
    fprintf(stderr, "free: ptr=%p, caller=%p\n", ptr, caller);

    dump_backtrace(15);
}

TEST(MallocHookTest, init) {
    set_malloc_hook(malloc_hook);
    set_realloc_hook(realloc_hook);
    set_free_hook(free_hook);

    void *p = malloc(100);

    ASSERT_EQ(last_malloc_ptr, p);
    ASSERT_EQ(last_malloc_size, 100);

    free(p);
}