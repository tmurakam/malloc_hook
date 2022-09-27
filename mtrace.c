/*
 * Memory Hook library for debugging
 * https://github.com/tmurakam/malloc_hook
 *
 * Copyright (c) 2022, Takuya Murakami.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, 
 *      this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdbool.h>

#include "malloc_hook.h"

static FILE *_fp = NULL;
static const char *_program_name;
static bool _resolve_symbol;
static int _max_stack_depth;
static bool _started = false;
static bool _need_close = false;

static void mtrace_start(const char *argv0, bool resolve_symbol, int max_stack_depth) {
    if (!_started) {
        _started = true;

        _program_name = argv0;
        _resolve_symbol = resolve_symbol;
        _max_stack_depth = max_stack_depth;

        fprintf(_fp, "= Start\n");
    }
}

static void mtrace_stop() {
    if (_started) {
        _started = false;
        fprintf(_fp, "= End\n");

        if (_need_close) {
            fclose(_fp);
            _fp = NULL;
            _need_close = false;
        }
    }
}


static void print_caller_symbol(void *caller[]) {
    char caller_symbol[1024];
    if (_resolve_symbol) {
        fprintf(_fp, " (");
        for (int i = 0; i < _max_stack_depth && i < MALLOC_MAX_BACKTRACE; i++) {
            if (i > 0) {
                fprintf(_fp, ", ");
            }
            if (!caller[i]) break;
            get_caller_symbol(caller[i], caller_symbol, sizeof(caller_symbol));
            fprintf(_fp, "%s", caller_symbol);
        }
        fprintf(_fp, ")\n");
    } else {
        fprintf(_fp, "\n");
    }
}

static void mtrace_malloc_hook(void *ptr, size_t size, void *caller[]) {
    fprintf(_fp, "@ %s:[%p] + %p 0x%zx", _program_name, caller[0], ptr, size);
    print_caller_symbol(caller);
}

static void mtrace_realloc_hook(void *oldPtr, size_t oldSize, void *newPtr, size_t newSize, void *caller[]) {
    fprintf(_fp, "@ %s:[%p] < %p", _program_name, caller[0], oldPtr);
    print_caller_symbol(caller);
    fprintf(_fp, "@ %s:[%p] > %p 0x%zx", _program_name, caller[0], newPtr, newSize);
    print_caller_symbol(caller);
}

static void mtrace_free_hook(void *ptr, size_t size, void *caller[]) {
    fprintf(_fp, "@ %s:[%p] - %p", _program_name, caller[0], ptr);
    print_caller_symbol(caller);
}

void malloc_hook_mtrace_fp(const char *argv0, FILE *fp, int resolve_symbol, int max_stack_depth) {
    _fp = fp;
    mtrace_start(argv0, resolve_symbol, max_stack_depth);
    set_malloc_hook(mtrace_malloc_hook);
    set_realloc_hook(mtrace_realloc_hook);
    set_free_hook(mtrace_free_hook);
}

void malloc_hook_mtrace(const char *argv0, const char *filename, int resolve_symbol, int max_stack_depth) {
    FILE *fp = fopen(filename, "w");
    _need_close = true;
    malloc_hook_mtrace_fp(argv0, fp, resolve_symbol, max_stack_depth);
}

void malloc_hook_muntrace() {
    set_malloc_hook(NULL);
    set_realloc_hook(NULL);
    set_free_hook(NULL);
    mtrace_stop();
}

