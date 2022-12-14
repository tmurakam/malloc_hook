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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*malloc_hook_t)(void *ptr, size_t size, void *caller_stack[]);
typedef void (*realloc_hook_t)(void *oldPtr, size_t oldSize, void *newPtr, size_t newSize, void *caller_stack[]);
typedef void (*free_hook_t)(void *ptr, size_t size, void *caller_stack[]);

// Max stacktrace depth to record.
// This affects memory overhead because of large memory header size.
#define MALLOC_MAX_BACKTRACE 5

/**
 * set malloc hook
 * @param hook Hook
 */
void set_malloc_hook(malloc_hook_t hook);

/**
 * set realloc hook
 * @param hook Hook
 */
void set_realloc_hook(realloc_hook_t hook);

/**
 * set free hook
 * @param hook Hook
 */
void set_free_hook(free_hook_t hook);

/**
 * Get total malloced size
 * @return size
 */
long get_malloc_total();

/**
 * Heap dump all heap.
 * If heap dump mark is set, only newer entry than the mark will be displayed.
 *
 * Note: If set resolve_symbols to true, this function malloc symbol info in this function.
 *
 * @param fp   Output stream of dump (stderr, etc)
 * @param resolve_symbols   Set true to resolve symbols.
 */
void malloc_heap_dump(FILE *fp, bool resolve_symbols);

/**
 * Mark heap dump point.
 * If the mark is set, only newer entry than the mark will be displayed by malloc_heap_dump().
 */
void malloc_heap_dump_mark();

/**
 * Unmark heap dump point.
 */
void malloc_heap_dump_unmark();

/**
 * Get caller symbol.
 *
 * @param caller Caller address
 * @param buffer Symbol buffer
 * @param buflen Length of the buffer
 */
void get_caller_symbol(void *caller, char *buffer, int buflen);

/**
 * dump backtrace
 * @param depth Max stack dump depth
 */
void dump_backtrace(int depth);

/**
 * start memory trace
 * @param argv0  Program name (argv[0])
 * @param filename  Log file name
 * @param resolve_symbol Set true to resolve caller symbol
 * @param max_stack_depth Max depth of stack trace to display
 */
void malloc_hook_mtrace(const char *argv0, const char *filename, int resolve_symbol, int max_stack_depth);

/**
 * start memory trace
 * @param argv0  Program name (argv[0])
 * @param fp  Log file pointer
 * @param resolve_symbol Set true to resolve caller symbol
 * @param max_stack_depth Max depth of stack trace to display
 */
void malloc_hook_mtrace_fp(const char *argv0, FILE *fp, int resolve_symbol, int max_stack_depth);

/**
 * stop memory trace
 */
void malloc_hook_muntrace();

#ifdef __cplusplus
};
#endif
