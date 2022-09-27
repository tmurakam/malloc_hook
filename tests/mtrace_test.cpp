#include <gtest/gtest.h>
#include <dlfcn.h>

#include "../malloc_hook.h"

TEST(MtraceTest, trace) {
    // filename
    malloc_hook_mtrace("./malloc_hook_test", "mtrace.log", true, 2);
    malloc_hook_muntrace();

    // fp
    malloc_hook_mtrace_fp("./malloc_hook_test", stderr, true, 2);

    void *p = malloc(100);
    p = realloc(p, 10000);
    free(p);

    malloc_hook_muntrace();
}
