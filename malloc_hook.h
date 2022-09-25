/*
 * Memory Hook library for debugging
 * https://github.com/tmurakam/malloc_hook
 *
 * Copyright (c) 2022, Takuya Murakami
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
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

typedef void (*malloc_hook_t)(void *ptr, size_t size, void *caller);
typedef void (*realloc_hook_t)(void *oldPtr, size_t oldSize, void *newPtr, size_t newSize, void *caller);
typedef void (*free_hook_t)(void *ptr, size_t size, void *caller);

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
 * dump backtrace
 * @param depth Max stack dump depth
 */
void dump_backtrace(int depth);

#ifdef __cplusplus
};
#endif
