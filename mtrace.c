#include <stdio.h>
//#include <dlfcn.h>

#include "malloc_hook.h"

static FILE *fp = NULL;
static const char *programName;

static void mtrace_start(const char *argv0, const char *filename) {
    if (!fp) {
        programName = argv0;
        fp = fopen(filename, "w");
        fprintf(fp, "= Start\n");
    }
}

static void mtrace_stop() {
    if (fp) {
        fprintf(fp, "= End\n");
        fclose(fp);
        fp = NULL;
    }
}

static void mtrace_malloc_hook(void *ptr, size_t size, void *caller) {
    fprintf(fp, "@ %s:[%p] + %p %zx\n", programName, caller, ptr, size);
}

static void mtrace_realloc_hook(void *oldPtr, size_t oldSize, void *newPtr, size_t newSize, void *caller) {
    fprintf(fp, "@ %s:[%p] < %p\n", programName, caller, oldPtr);
    fprintf(fp, "@ %s:[%p] > %p 0x%zx\n", programName, caller, newPtr, newSize);
}

static void mtrace_free_hook(void *ptr, size_t size, void *caller) {
    fprintf(fp, "@ %s:[%p] - %p\n", programName, caller, ptr);
}

void malloc_hook_mtrace(const char *argv0, const char *filename) {
    mtrace_start(argv0, filename);
    set_malloc_hook(mtrace_malloc_hook);
    set_realloc_hook(mtrace_realloc_hook);
    set_free_hook(mtrace_free_hook);
}

void malloc_hook_muntrace() {
    set_malloc_hook(NULL);
    set_realloc_hook(NULL);
    set_free_hook(NULL);
    mtrace_stop();
}

