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

void realloc_hook(void *ptr, void *newptr, size_t size, void *caller) {
    fprintf(stderr, "realloc: ptr=%p, newptr=%p, size=%ld, caller=%p\n", ptr, size, caller);
}
void free_hook(void *ptr, void *caller) {
    fprintf(stderr, "free: ptr=%p, caller=%p\n", ptr, caller);
}

int main() {
    set_malloc_hook(malloc_hook);
    set_realloc_hook(realloc_hook);
    set_free_hook(free_hook);
    
    // Your code here...
}
```
