# forge

A convention-based build tool for C and C++ projects. Project structure implies build instructions — declare library dependencies and forge derives everything else from the directory layout.

## Build forge

```sh
cc -std=c11 -O2 -o forge forge.c
```

## Commands

| Command | Description |
|---------|-------------|
| `forge build` | Compile all libraries and bindings |
| `forge test` | Build then run all test suites |
| `forge clean` | Remove all build outputs |

---

## Project layouts

### Single-library or executable

```
project/
├── project.toml
├── include/        global headers (added to -I for all compilations)
├── src/            compiled into libNAME.a, or executable if main.c present
├── bindings/       Python extension module (optional)
└── tests/          test suite (optional)
```

### Multi-library

```
project/
├── project.toml
├── include/
└── libs/
    └── NAME/
        ├── src/        compiled into .build/lib/libNAME.a
        ├── bindings/   Python extension module (optional)
        └── tests/      test suite for this library (optional)
```

---

## project.toml

### Top-level fields

| Field      | Default     | Description |
|------------|-------------|-------------|
| `name`     | `"project"` | Project name, used to name build outputs |
| `compiler` | `"cc"`      | Compiler binary (`cc`, `gcc`, `clang`, `g++`, `clang++`, …) |
| `std`      | `"c11"`     | Language standard (`c11`, `c17`, `c++17`, `c++20`, …) |
| `python`   | `"python3"` | Python interpreter for bindings and pytest |
| `werror`   | `"true"`    | Pass `-Werror`; set `"false"` to disable |

### `[[lib]]` entries (multi-library projects)

Each `[[lib]]` block declares one library:

| Field  | Default         | Description |
|--------|-----------------|-------------|
| `name` | *(required)*    | Library name. Output: `.build/lib/libNAME.a` |
| `src`  | `libs/NAME/src` | Source directory |
| `deps` | `[]`            | Names of other libs this one depends on |

A `[[lib]]` with no `src` field and no `libs/NAME/src` directory on disk is a **grouping lib** — it participates in dependency ordering but produces no compiled output.

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

## Build outputs

```
.build/
├── obj/        compiled object files
├── deps/       .d dependency files (for incremental build tracking)
├── lib/        static libraries (.a) and Python extension modules (.so)
└── bin/
    ├── NAME        executable (single-lib mode, when main.c present)
    └── tests/      native test binaries
```

---

## Python bindings

Place binding sources in `bindings/` next to `src/`. forge auto-detects the binding type by scanning source files:

| Marker | Type | Compilation flags |
|--------|------|-------------------|
| `PYBIND11_MODULE` | pybind11 | `-fPIC -fexceptions -frtti` + Python/pybind11 headers |
| `ffi.cdef` or `cffi` | cffi | `-fPIC` |
| *(none)* | ctypes | `-fPIC` |

For pybind11, the module name is extracted directly from the `PYBIND11_MODULE(name, ...)` macro. The output `.so` is named to match what Python's import system expects (using `sysconfig.get_config_var('EXT_SUFFIX')`).

After building, the `.so` is copied into `tests/` so pytest can import it directly without any `PYTHONPATH` changes.

Requirements:
- pybind11 bindings: `pip install pybind11`
- Python tests: `pip install pytest`

**Note:** `-Werror` is never applied to binding sources, since third-party headers may trigger warnings.

---

## Testing

`forge test` runs `forge build` first, then for each library with a `tests/` directory:

- **Native tests** — each `.c`/`.cpp` file is compiled and linked as a standalone binary, run, and reported pass/fail by exit code.
- **Python tests** — all `.py` files are run together under a single `pytest -v --tb=short .` invocation from inside `tests/`.

Both contribute to the final pass/fail count. `forge test` exits non-zero if any test fails.

---

## Worked examples

### Hello world executable

```
hello/
├── project.toml
└── src/
    └── main.c
```

```toml
# project.toml
name     = "hello"
compiler = "cc"
std      = "c11"
werror   = "false"
```

```c
/* src/main.c */
#include <stdio.h>
int main(void) { puts("hello, world"); return 0; }
```

```sh
forge build   # produces .build/bin/hello
.build/bin/hello
```

---

### Multi-library with tests

```
myproject/
├── project.toml
├── include/
│   ├── math.h
│   └── calc.h
└── libs/
    ├── math/
    │   └── src/math.c
    └── calc/
        ├── src/calc.c
        └── tests/test_calc.c
```

```toml
# project.toml
name     = "myproject"
compiler = "cc"
std      = "c11"

[[lib]]
name = "math"

[[lib]]
name = "calc"
deps = ["math"]
```

```sh
forge test    # builds math, then calc, then runs tests/test_calc as a binary
```

---

## Limitations

- **No incremental builds.** `.d` files are emitted but not consumed — every `forge build` recompiles all sources.
- **No package management.** Third-party dependencies must be handled outside forge.
- **`bindings/` is flat.** Binding sources must live directly in `bindings/`; subdirectories are not walked.
- **Single `bindings/` per lib.** One Python extension module per library.
- **POSIX only.** Uses `popen`, `opendir`, and other POSIX APIs. Windows is not supported.
