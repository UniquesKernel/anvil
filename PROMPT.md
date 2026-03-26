# forge — Ralph Loop Build Prompt

## Task

Implement `forge` — a convention-based build tool for C and C++ projects — as a single C file: `forge.c`.

It must compile cleanly with:

```
cc -std=c11 -O2 -o forge forge.c
```

with no warnings and no errors.

---

## What to do each iteration

1. Read `forge-spec.md` fully to understand the expected behavior.
2. Read the current state of `forge.c` if it exists.
3. Check which acceptance conditions in `forge-spec.md` are already satisfied (you can tell by reading the code).
4. Pick the next unsatisfied acceptance condition (or logical group of them) and implement it.
5. After editing `forge.c`, compile it with `cc -std=c11 -O2 -o forge forge.c` and fix any errors or warnings before stopping.
6. Do NOT do everything at once — make focused, incremental progress each iteration.

---

## Completion criteria

You are done when **every acceptance condition** listed in the `## Acceptance conditions` section of `forge-spec.md` is implemented and the following all pass:

- `cc -std=c11 -O2 -o forge forge.c` produces zero warnings and zero errors
- All checklist items under every heading in `## Acceptance conditions` are satisfied by the code

When you are confident all conditions are met, output exactly:

```
<promise>FORGE COMPLETE</promise>
```

Do NOT output this promise until every acceptance condition is genuinely implemented and the binary compiles cleanly.

---

## Rules

- All code goes in a single file: `forge.c`
- Target: C11, POSIX only (use `popen`, `opendir`, `mmap`, etc.)
- No external libraries beyond the C standard library and POSIX
- Follow the compiler invocation formats in `forge-spec.md` exactly
- Respect all defaults documented in the `project.toml` fields table
- Do not invent features not in the spec
