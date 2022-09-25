#include <gtest/gtest.h>
#include <dlfcn.h>

#include "../malloc_hook.h"

static void *last_malloc_ptr;
static int last_malloc_size;

void malloc_hook(void *ptr, size_t size, void *caller) {
    fprintf(stderr, "malloc: ptr=%p, size=%ld, caller=%p\n", ptr, size, caller);
    last_malloc_ptr = ptr;
    last_malloc_size = size;

    //dump_backtrace(15);
}

void realloc_hook(void *oldPtr, size_t oldSize, void *newPtr, size_t newSize, void *caller) {
    fprintf(stderr, "realloc: oldPtr=%p, oldSize=%ld, newPtr=%p, newSize=%ld, caller=%p\n", oldPtr, oldSize, newPtr, newSize, caller);

    //dump_backtrace(15);
}

void free_hook(void *ptr, size_t size, void *caller) {
    fprintf(stderr, "free: ptr=%p, size=%ld, caller=%p\n", ptr, size, caller);

    //dump_backtrace(15);
}

class HookSetUp {
public:
    HookSetUp() {
        set_malloc_hook(malloc_hook);
        set_realloc_hook(realloc_hook);
        set_free_hook(free_hook);
    }
};

static HookSetUp _hookSetUp;

TEST(MallocHookTest, init) {
    long initial = get_malloc_total();

    void *p = malloc(100);

    ASSERT_EQ(last_malloc_ptr, p);
    ASSERT_EQ(last_malloc_size, 100);
    ASSERT_EQ(get_malloc_total(), initial + 100);

    free(p);

    ASSERT_EQ(get_malloc_total(), initial);
}

TEST(MallocHookTest, realloc) {
    long initial = get_malloc_total();

    // realloc as malloc
    unsigned char *p = (unsigned char *)realloc(NULL, 10);
    for (int i = 0; i < 10; i++) {
        p[i] = i;
    }

    ASSERT_EQ(get_malloc_total(), initial + 10);

    void *p2 = malloc(200); // dummy, to move realloc area

    // realloc (address will be moved)
    p = (unsigned char *)realloc(p, 100000);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(p[i], i);
    }

    ASSERT_EQ(get_malloc_total(), initial + 200 + 100000);

    // realloc again
    p = (unsigned char *)realloc(p, 300);
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(p[i], i);
    }

    ASSERT_EQ(get_malloc_total(), initial + 200 + 300);

    free(p2);
    free(p);

    ASSERT_EQ(get_malloc_total(), initial);
}