#include <gtest/gtest.h>
#include <dlfcn.h>

#include "../malloc_hook.h"

TEST(MtraceTest, trace) {
    malloc_hook_mtrace("./malloc_hook_test", "mtrace.log");

    void *p = malloc(100);
    p = realloc(p, 10000);
    //free(p);

    malloc_hook_muntrace();
}