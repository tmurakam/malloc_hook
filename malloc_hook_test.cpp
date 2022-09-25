#include <gtest/gtest.h>
#include <dlfcn.h>
#include "malloc_hook.h"

static void *last_malloc_ptr;
static int last_malloc_size;

void malloc_hook(void *ptr, size_t size, void *caller) {
    Dl_info info;
    dladdr(caller, &info);
    printf("malloc: ptr=%p, size=%ld, caller=%p [%s]\n", ptr, size, caller, info.dli_sname);
    last_malloc_ptr = ptr;
    last_malloc_size = size;
}

void realloc_hook(void *ptr, void *newptr, size_t size, void *caller) {
    Dl_info info;
    dladdr(caller, &info);
    printf("realloc: ptr=%p, newptr=%p, size=%ld, caller=%p [%s]\n", ptr, newptr, size, caller, info.dli_sname);
}

void free_hook(void *ptr, void *caller) {
    Dl_info info;
    dladdr(caller, &info);
    printf("free: ptr=%p, caller=%p [%s]\n", ptr, caller, info.dli_sname);
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