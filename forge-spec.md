# forge specification

## Overview

forge is a convention-based build tool for C and C++ projects. Its core
philosophy is that project structure implies build instructions — the user
declares dependencies between libraries and forge derives everything else
from the directory layout. No build graph construction. No per-target
configuration.

---

## Project structure

### Multi-library project

```
project/
├── project.toml
├── include/                    global headers, added to -I for all compilations
├── libs/
│   └── NAME/
│       ├── src/                compiled into .build/lib/libNAME.a
│       ├── bindings/           Python extension module (optional)
│       └── tests/              test suite for this library (optional)
└── apps/
    └── NAME/
        └── src/                compiled and linked into .build/bin/NAME
```

### Single-library / executable project (no [[lib]] entries)

```
project/
├── project.toml
├── include/
├── src/                        compiled into libNAME.a, or executable if main.c present
├── bindings/                   Python extension module (optional)
└── tests/                      test suite (optional)
```

---

## project.toml

### Top-level fields

| Field      | Type   | Default     | Description                              |
|------------|--------|-------------|------------------------------------------|
| `name`     | string | `"project"` | Project name, used to name build outputs |
| `compiler` | string | `"cc"`      | Compiler binary: cc, gcc, clang, g++, clang++, … |
| `std`      | string | `"c11"`     | Language standard: c11, c17, c++17, c++20, … |
| `python`   | string | `"python3"` | Python interpreter used for bindings and pytest |
| `werror`   | bool   | `"true"`    | Whether to pass -Werror. Set `"false"` to disable |

### [[lib]] entries

Each `[[lib]]` entry declares one library. Multiple entries are allowed.

| Field  | Type         | Default           | Description                                     |
|--------|--------------|-------------------|-------------------------------------------------|
| `name` | string       | required          | Library name. Output: `.build/lib/libNAME.a`    |
| `src`  | string       | `libs/NAME/src`   | Source directory for this library               |
| `deps` | string array | `[]`              | Names of other `[[lib]]` entries this lib depends on |

A `[[lib]]` with no `src` field and no `libs/NAME/src` directory is a
**grouping lib** — it produces no compiled output but participates in
dependency resolution and link ordering.

### [[exe]] entries

Each `[[exe]]` entry declares one executable. Multiple entries are allowed.

| Field  | Type         | Default           | Description                                     |
|--------|--------------|-------------------|-------------------------------------------------|
| `name` | string       | required          | Executable name. Output: `.build/bin/NAME`      |
| `src`  | string       | `apps/NAME/src`   | Source directory for this executable            |
| `deps` | string array | `[]`              | Names of `[[lib]]` entries to link against      |

Executables are built after all libraries. Their `deps` may only reference
`[[lib]]` entries, not other `[[exe]]` entries.

### Example

```toml
name     = "anvil"
compiler = "clang++"
std      = "c++20"
werror   = "true"
python   = "python3"

[[lib]]
name = "math"
src  = "libs/math/src"

[[lib]]
name = "memory"
src  = "libs/memory/src"
deps = ["math"]

[[lib]]
name = "graphic"
src  = "libs/graphic/src"
deps = ["memory"]
```

---

## Conventions

### Library root inference

The library root is derived from the `src` field by stripping the last
path component:

```
src = "libs/memory/src"  →  root = "libs/memory"
```

forge then checks for `root/bindings/` and `root/tests/` automatically.
No additional configuration is required.

### Executable detection

In single-lib mode, if `src/` contains a file named `main.c`, `main.cpp`,
or `main.cc`, forge produces an executable at `.build/bin/NAME` instead
of a static library.

### Binding type detection

forge scans source files in `bindings/` for known markers:

| Marker found            | Binding type | Notes                                    |
|-------------------------|--------------|------------------------------------------|
| `PYBIND11_MODULE`       | pybind11     | Module name extracted from macro         |
| `ffi.cdef` or `cffi`    | cffi         | Plain shared library, no Python headers  |
| (none of the above)     | ctypes       | Plain shared library, no Python headers  |

For pybind11 bindings, forge extracts the module name directly from the
`PYBIND11_MODULE(name, ...)` macro. The name determines the output
filename so Python's import machinery can find it.

### Binding dependencies

A library's bindings are linked against the same set of libraries as the
library itself — the lib plus its full transitive dependency closure. This
is automatic and not configurable.

### .so placement

After building, the `.so` file is copied into the lib's `tests/` directory
so pytest can import it directly without any `PYTHONPATH` manipulation.

---

## Build outputs

```
.build/
├── obj/        compiled object files (flat, path separators replaced with _)
├── deps/       .d dependency files (emitted via -MMD -MF, reserved for incremental builds)
├── lib/        static libraries (.a) and Python extension modules (.so)
└── bin/
    ├── NAME            executable (single-lib mode or [[exe]] entries)
    └── tests/          native test binaries
```

---

## Warning flags

forge enables an aggressive warning set by default. Flags are split by
compiler family and applied automatically based on the `compiler` field.

### Common (gcc and clang)

```
-Wall -Wextra -Wpedantic
-Wconversion -Wsign-conversion -Wshadow
-Wformat=2 -Wformat-security
-Wnull-dereference -Wdouble-promotion
-Wcast-align -Wcast-qual
-Wimplicit-fallthrough
-Wswitch-enum -Wswitch-default
-Wmissing-declarations -Wmissing-noreturn
-Wvla -Wstrict-overflow=2 -Wfloat-equal -Wundef
-Wpointer-arith -Wwrite-strings
-fstack-protector-strong
```

### GCC only

```
-Wduplicated-cond -Wduplicated-branches
-Wlogical-op -Wformat-signedness
-Warray-bounds=2 -Wuseless-cast
```

### Clang only

```
-Wunreachable-code -Wunreachable-code-break
-Wunreachable-code-return -Wloop-analysis
-Wconditional-uninitialized -Wcomma
-Wover-aligned -Warray-bounds
```

### Bindings

`-Werror` is always suppressed for `bindings/` sources regardless of the
`werror` setting. pybind11, cffi, and ctypes headers are third-party code
and may trip warnings the project does not own. All other warning flags
still apply.

---

## Dependency resolution

### Validation

Before any compilation, forge verifies that every name in every `deps`
array refers to a declared `[[lib]]` entry. A missing name is a hard
error.

### Build order

forge performs a topological sort (Kahn's algorithm) over the `[[lib]]`
entries. Libraries with no unsatisfied dependencies are built first.
A cycle in the dependency graph is a hard error.

### Link order

Linker arguments are emitted in reverse topological order. The most
dependent library appears first on the command line; the least dependent
appears last. This is required for correct symbol resolution with static
archives.

---

## Commands

### `forge build`

1. Load and validate `project.toml`
2. Topologically sort `[[lib]]` entries
3. For each lib in order:
   - Compile all `.c` / `.cpp` / `.cc` / `.cxx` files in `src/` into object files
   - Archive objects into `.build/lib/libNAME.a`
   - If `root/bindings/` exists: detect binding type, compile and link `.so`,
     copy into `root/tests/`
4. In single-lib mode: compile `src/`, produce library or executable,
   then handle `bindings/` if present
5. For each `[[exe]]` entry: compile `src/`, link against dep closure into `.build/bin/NAME`

### `forge test`

1. Run `forge build`
2. For each lib in topological order, if `root/tests/` exists:
   - **Native tests**: compile each `.c` / `.cpp` file as a standalone binary
     linked against the lib's transitive dep closure, run it, report pass/fail
   - **Python tests**: run `pytest -v --tb=short .` inside `root/tests/`
3. Report total pass/fail count across all suites

### `forge clean`

- Remove `.build/`
- Remove all `*.so` files found inside any `tests/` directory in the tree

---

## Compiler invocation

### Object compilation

```
COMPILER STD WARN_FLAGS INC_FLAGS EXTRA -MMD -MF DEPFILE -c SOURCE -o OBJECT
```

- `STD` — e.g. `-std=c++20`
- `INC_FLAGS` — `-I include` if `include/` exists, plus Python/pybind11 headers for bindings
- `EXTRA` — `-fPIC -fexceptions -frtti` for pybind11 bindings; `-fPIC` for ctypes/cffi

### Static library

```
ar rcs .build/lib/libNAME.a OBJECTS...
```

### Python extension

```
COMPILER -shared OBJECTS LINK_ARGS [PY_LDFLAGS] -o .build/lib/MODULE.EXT_SUFFIX
```

`EXT_SUFFIX` is obtained from `sysconfig.get_config_var('EXT_SUFFIX')` so
the output filename matches what Python's import system expects
(e.g. `.cpython-312-x86_64-linux-gnu.so`).

### Native executable (single-lib mode)

```
COMPILER OBJECTS -o .build/bin/NAME
```

### Native test binary

```
COMPILER OBJECT LINK_ARGS -o .build/bin/tests/TESTNAME
```

---

## Acceptance conditions

The implementation is considered correct when all of the following hold.

### project.toml parsing

- [ ] A missing `project.toml` produces a clear error and a non-zero exit code
- [ ] All top-level fields fall back to their documented defaults when absent
- [ ] A `[[lib]]` entry with no `name` field produces a hard error
- [ ] A `[[lib]]` entry with no `src` field and no `libs/NAME/src` directory is treated as a grouping lib and produces no compilation
- [ ] `deps = [...]` entries with zero, one, and multiple items all parse correctly
- [ ] A `deps` entry referencing a name not declared as a `[[lib]]` produces a hard error before any compilation begins

### build order

- [ ] Libraries with no dependencies are built before libraries that depend on them
- [ ] A project with a linear chain `A → B → C` builds in order `A, B, C`
- [ ] A project with a diamond `A → B, A → C, D → B, D → C` builds `B` and `C` before `D`
- [ ] A cycle between any two or more `[[lib]]` entries produces a hard error before any compilation begins
- [ ] The build order is printed to stdout before compilation starts

### compilation

- [ ] All `.c`, `.cpp`, `.cc`, and `.cxx` files under `src/` are compiled, including files in subdirectories
- [ ] Files in subdirectories of `src/` produce distinct object file names with no collisions
- [ ] `-MMD -MF` dependency files are emitted into `.build/deps/` for every compiled translation unit
- [ ] `include/` is added to `-I` for every compilation if the directory exists
- [ ] The compiler family is detected from the `compiler` field and the correct warning flag set is applied
- [ ] `-Werror` is applied when `werror = "true"` and suppressed when `werror = "false"`
- [ ] A compilation failure produces a clear error identifying the failing source file and returns a non-zero exit code without attempting to continue

### linking

- [ ] Each lib is archived into `.build/lib/libNAME.a`
- [ ] Linker arguments for a lib include that lib and its full transitive dep closure
- [ ] Linker arguments are in reverse topological order
- [ ] A lib that has never been compiled does not appear in link arguments even if declared as a dep

### Python bindings

- [ ] A `bindings/` directory containing `PYBIND11_MODULE` is detected as pybind11
- [ ] A `bindings/` directory containing `ffi.cdef` or `cffi` is detected as cffi
- [ ] A `bindings/` directory with neither marker is treated as ctypes
- [ ] The pybind11 module name is extracted from `PYBIND11_MODULE(name, ...)` and used as the output filename
- [ ] The `.so` suffix is obtained from `sysconfig.get_config_var('EXT_SUFFIX')` so the output filename matches what Python's import system expects
- [ ] pybind11 bindings are compiled with `-fexceptions -frtti -fPIC`
- [ ] ctypes and cffi bindings are compiled with `-fPIC` only
- [ ] `-Werror` is never passed when compiling binding sources regardless of the `werror` setting
- [ ] The built `.so` is copied into the lib's `tests/` directory after a successful link
- [ ] A missing `pybind11` installation produces a clear error suggesting `pip install pybind11`
- [ ] Binding link args are identical to the link args of the lib they belong to

### testing

- [ ] Each `.c` / `.cpp` file in `tests/` compiles and links into an independent binary
- [ ] Each native test binary is linked against the lib's full transitive dep closure
- [ ] A native test binary that exits 0 is reported as passing
- [ ] A native test binary that exits non-zero is reported as failing with its exit code
- [ ] All `.py` files in `tests/` are run together under a single `pytest` invocation per lib
- [ ] pytest is invoked from inside `tests/` so relative imports and the copied `.so` resolve correctly
- [ ] A missing `pytest` installation produces a clear error suggesting `pip install pytest`
- [ ] Native and Python tests are both run and both contribute to the final pass/fail count
- [ ] `forge test` returns a non-zero exit code if any test suite fails

### clean

- [ ] `forge clean` removes `.build/`
- [ ] `forge clean` removes all `*.so` files found inside any `tests/` directory in the tree
- [ ] `forge clean` is idempotent — running it twice produces no error

### general

- [ ] An unknown command produces a clear error and a non-zero exit code
- [ ] Running `forge` with no arguments prints usage without error
- [ ] The single-lib fallback (no `[[lib]]` entries, `src/` present) builds and tests correctly
- [ ] The executable fallback (no `[[lib]]` entries, `main.c` present in `src/`) produces `.build/bin/NAME`
- [ ] forge itself compiles cleanly with `cc -std=c11 -O2 -o forge forge.c` with no warnings

## Current limitations

- **No incremental builds.** `.d` files are emitted but not yet consumed.
  Every `forge build` recompiles all sources.
- **No package management.** Third-party dependencies must be handled
  outside forge (system packages, vendored source, etc.)
- **`bindings/` is flat.** forge does not recurse into subdirectories of
  `bindings/`. All binding sources must live directly in that directory.
  `src/` and `tests/` are walked recursively.
- **Single `bindings/` per lib.** A library cannot have multiple
  independent Python extension modules.
- **POSIX only.** forge uses `popen`, `system`, `mmap`, and POSIX
  directory APIs. Windows is not supported.
