# malloc_hook library

This is a debugging library for malloc.
You can set 'hooks' for malloc, realloc and free.

This library is thread safe.

## Build

    $ mkdir build && cd build
    $ cmake ..
    $ make
 
## How to use

Just link `libmalloc_hook.so` to your application.
Or, add `malloc_hook.c` and `malloc_hook.h` files to your application.

You can set hooks for malloc, realloc, and free.
Here is an example.

```c
#include "mallok_hook.h"

void malloc_hook(void *ptr, size_t size, void *caller) {
    fprintf(stderr, "malloc: ptr=%p, size=%ld, caller=%p\n", ptr, size, caller);
    dump_backtrace(15);
}

void realloc_hook(void *oldPtr, size_t oldSize, void *newPtr, size_t newSize, void *caller) {
    fprintf(stderr, "realloc: oldPtr=%p, oldSize=%ld, newPtr=%p, newSize=%ld, caller=%p\n", oldPtr, oldSize, newPtr, newSize, caller);
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

## Notes

* Before calling the hooks, this library takes mutex lock to ensure thread safety.
* You can use all malloc functions in the hook, but hooks are not called recursively.
