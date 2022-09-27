# ChangeLogs

## v0.0.4 - 2022/09/27

- Add heap dump mark/unmark feature.

## v0.0.3 - 2022/09/27

- Support to record caller stack frames, not only 1 caller.
  Max depth of the stack frames are defined as MALLOC_MAX_BACKTRACE.
- BREAKING CHANGES 
  - Update all hook i/f, caller argument is changed to array of pointers.
  - malloc_hook_mtrace(): add max_stack_depth argument.
- Add malloc_hook_mtrace_fp()

## v0.0.2 - 2022/09/26

- Add malloc_heap_dump()

## v0.0.1 - 2022/09/26

- Initial release
