# Malloc hook library

## What is this?

This is a debugging library for malloc.
You can set 'hooks' for `malloc`, `realloc` and `free`.

This is designed to replace the deprecated `__malloc_hook()` of glibc. 

Unlike the `mtrace`, this is fully thread safe, so you can use this in multi threaded application,

## Supported OS

Linux only.

## Prerequisite

* CMake >= 3.14
* g++

## Build

    $ mkdir build && cd build
    $ cmake ..
    $ make
 
## How to use

Just link `libmalloc_hook.so` to your application.
Or, add `malloc_hook.c` and `malloc_hook.h` files to your application.

You can set hooks for `malloc`, `realloc`, and `free`.
See `malloc_hook.h` for full API documents.

Here is an example.

```c
#include "mallok_hook.h"

void malloc_hook(void *ptr, size_t size, void *caller) {
    fprintf(stderr, "malloc: ptr=%p, size=%ld, caller=%p\n", ptr, size, caller);
    dump_backtrace(15);
}

void realloc_hook(void *oldPtr, size_t oldSize, void *newPtr, size_t newSize, void *caller) {
    fprintf(stderr, "realloc: oldPtr=%p, oldSize=%ld, newPtr=%p, newSize=%ld, caller=%p\n", 
            oldPtr, oldSize, newPtr, newSize, caller);
}

void free_hook(void *ptr, size_t size, void *caller) {
    fprintf(stderr, "free: ptr=%p, size=%ld, caller=%p\n", ptr, size, caller);
}

int main() {
    set_malloc_hook(malloc_hook);
    set_realloc_hook(realloc_hook);
    set_free_hook(free_hook);
    
    // Your code here...
}
```

### Notes

* Before calling the hooks, this library takes mutex lock to ensure thread safety.
* You can use all `malloc` related functions in the hook, but hooks are not called recursively.
* The `calloc` calls `malloc` internally.
* A small memory header (8 bytes) are inserted at head of allocated memory. This is used to track size of the memory.

Note: If you want to get caller's filename and line number, you need to disable ASLR (address space layout randomization).
Also you need to calculate address offset, and use `addr2line` utility.

Otherwise, you can use `get_caller_symbol()` in your hook to get program address.

## mtrace utility

This library provides `mtrace` like functionality too. This is thread safe.

Use `memory_hook_mtrace` and `memory_hook_muntrace` instead of `mtrace` and `muntrace`.
See `mtrace_test.cpp` for details.

You can use `mtrace` utility to analyze mtrace log file.
