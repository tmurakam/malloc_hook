#include <stdio.h>
#include <stdbool.h>

#include "malloc_hook.h"

static FILE *fp = NULL;
static const char *programName;
static char caller_symbol[1024];
static bool _resolve_symbol;

static void mtrace_start(const char *argv0, const char *filename, bool resolve_symbol) {
    if (!fp) {
        programName = argv0;
        _resolve_symbol = resolve_symbol;
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


static void print_caller_symbol(void *caller) {
    if (_resolve_symbol) {
        get_caller_symbol(caller, caller_symbol, sizeof(caller_symbol));
        fprintf(fp, "  (%s)\n", caller_symbol);
    } else {
        fprintf(fp, "\n");
    }
}

static void mtrace_malloc_hook(void *ptr, size_t size, void *caller) {
    fprintf(fp, "@ %s:[%p] + %p 0x%zx", programName, caller, ptr, size);
    print_caller_symbol(caller);
}

static void mtrace_realloc_hook(void *oldPtr, size_t oldSize, void *newPtr, size_t newSize, void *caller) {
    fprintf(fp, "@ %s:[%p] < %p", programName, caller, oldPtr);
    print_caller_symbol(caller);
    fprintf(fp, "@ %s:[%p] > %p 0x%zx", programName, caller, newPtr, newSize);
    print_caller_symbol(caller);
}

static void mtrace_free_hook(void *ptr, size_t size, void *caller) {
    fprintf(fp, "@ %s:[%p] - %p", programName, caller, ptr);
    print_caller_symbol(caller);
}

void malloc_hook_mtrace(const char *argv0, const char *filename, int resolve_symbol) {
    mtrace_start(argv0, filename, resolve_symbol);
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

