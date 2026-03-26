#!/bin/bash
# Validation test suite for forge
# Run from inside forge-test/: bash run-tests.sh

set -uo pipefail
cd "$(dirname "$0")"

PASS=0
FAIL=0
FORGE="$(pwd)/forge"

green() { printf '\033[32m%s\033[0m\n' "$*"; }
red()   { printf '\033[31m%s\033[0m\n' "$*"; }

expect_success() {
    local label="$1"; shift
    if "$@" > /dev/null 2>&1; then
        green "PASS $label"
        PASS=$((PASS+1))
    else
        red  "FAIL $label (expected exit 0)"
        FAIL=$((FAIL+1))
    fi
}

expect_failure() {
    local label="$1"; shift
    if ! "$@" > /dev/null 2>&1; then
        green "PASS $label"
        PASS=$((PASS+1))
    else
        red  "FAIL $label (expected non-zero exit)"
        FAIL=$((FAIL+1))
    fi
}

expect_output() {
    local label="$1" pattern="$2"; shift 2
    local out
    out=$("$@" 2>&1 || true)
    if echo "$out" | grep -q "$pattern"; then
        green "PASS $label"
        PASS=$((PASS+1))
    else
        red  "FAIL $label (pattern '$pattern' not found in output)"
        echo "  output was: $out"
        FAIL=$((FAIL+1))
    fi
}

expect_file() {
    local label="$1" path="$2"
    if [ -f "$path" ]; then
        green "PASS $label"
        PASS=$((PASS+1))
    else
        red  "FAIL $label (file not found: $path)"
        FAIL=$((FAIL+1))
    fi
}

expect_no_file() {
    local label="$1" path="$2"
    if [ ! -e "$path" ]; then
        green "PASS $label"
        PASS=$((PASS+1))
    else
        red  "FAIL $label (file should not exist: $path)"
        FAIL=$((FAIL+1))
    fi
}

# ── helpers ──────────────────────────────────────────────────────────────────

TMPDIR_ROOT="$(mktemp -d)"
cleanup() { rm -rf "$TMPDIR_ROOT"; }
trap cleanup EXIT

run_in() { local dir="$1"; shift; (cd "$dir" && "$@"); }

use_fixture() {
    local name="$1"
    local dst="$TMPDIR_ROOT/$name"
    cp -r "$(pwd)/tests/$name" "$dst"
    echo "$dst"
}

# ── 1. self-compile ───────────────────────────────────────────────────────────
echo ""
echo "=== self-compile ==="
expect_success "forge compiles with cc -std=c11 -O2" \
    cc -std=c11 -O2 -o "$FORGE" forge.c

# ── 2. general ────────────────────────────────────────────────────────────────
echo ""
echo "=== general ==="
expect_output  "no-args prints usage"          "usage:" $FORGE
expect_success "no-args exits 0"               $FORGE
expect_failure "unknown command exits non-zero" $FORGE unknowncmd
expect_output  "unknown command error message" "unknown command" $FORGE badcmd

# ── 3. project.toml parsing ───────────────────────────────────────────────────
echo ""
echo "=== project.toml ==="

D=$(use_fixture missing-toml)
expect_failure "missing project.toml exits non-zero" run_in "$D" $FORGE build
expect_output  "missing project.toml error message" "project.toml" run_in "$D" $FORGE build

D=$(use_fixture noname)
expect_failure "lib with no name field → error" run_in "$D" $FORGE build
expect_output  "lib with no name error message" "no 'name'" run_in "$D" $FORGE build

D=$(use_fixture baddep)
expect_failure "unknown dep → error" run_in "$D" $FORGE build
expect_output  "unknown dep message" "not declared" run_in "$D" $FORGE build
expect_no_file "unknown dep: no compilation attempted" "$D/.build"

# ── 4. build order ────────────────────────────────────────────────────────────
echo ""
echo "=== build order ==="

D=$(use_fixture cycle)
expect_failure "cycle → error" run_in "$D" $FORGE build
expect_output  "cycle message" "cycle" run_in "$D" $FORGE build
expect_no_file "cycle: no .build created" "$D/.build"

D=$(use_fixture linear)
expect_success "linear chain builds" run_in "$D" $FORGE build
expect_output  "linear chain order a b c" "Build order: a b c" run_in "$D" $FORGE build

D=$(use_fixture diamond)
expect_success "diamond builds" run_in "$D" $FORGE build
out=$(run_in "$D" $FORGE build 2>&1)
b_pos=$(echo "$out" | grep -o "Build order:.*" | head -1)
echo "  build order: $b_pos"
if echo "$b_pos" | grep -qP "b.*c.*d|c.*b.*d"; then
    green "PASS diamond: b and c before d"
    ((PASS++))
else
    red  "FAIL diamond: b and c before d — got: $b_pos"
    ((FAIL++))
fi

# ── 5. compilation ────────────────────────────────────────────────────────────
echo ""
echo "=== compilation ==="

D=$(use_fixture single-lib)
expect_success "single-lib builds" run_in "$D" $FORGE build
expect_file    ".a archive created" "$D/.build/lib/libmylib.a"
expect_file    "object file created" "$D/.build/obj/src_mylib_c.o"
expect_file    "dep file created"    "$D/.build/deps/src_mylib_c.o.d"

D=$(use_fixture subdirs)
expect_success "subdirectory sources compile" run_in "$D" $FORGE build
expect_file    "subdir a object distinct" "$D/.build/obj/src_a_util_c.o"
expect_file    "subdir b object distinct" "$D/.build/obj/src_b_util_c.o"

D=$(use_fixture exec)
expect_success "executable builds" run_in "$D" $FORGE build
expect_file    "executable produced" "$D/.build/bin/hello"

D=$(use_fixture werror-off)
expect_success "werror=false: warning doesn't fail build" run_in "$D" $FORGE build

D=$(use_fixture grouping)
expect_success "grouping lib builds (no error)" run_in "$D" $FORGE build
expect_no_file "grouping lib: no .a produced" "$D/.build/lib/libmeta.a"

# ── 6. linking ────────────────────────────────────────────────────────────────
echo ""
echo "=== linking ==="

D=$(use_fixture multi-test)
expect_success "transitive dep linking works" run_in "$D" $FORGE test

D=$(use_fixture grouping-dep)
expect_success "grouping-dep: test links without phantom lib" run_in "$D" $FORGE test

# ── 7. testing ────────────────────────────────────────────────────────────────
echo ""
echo "=== testing ==="

D=$(use_fixture with-tests)
expect_success "passing native test exits 0" run_in "$D" $FORGE test

D=$(use_fixture failing-test)
expect_failure "failing native test exits non-zero" run_in "$D" $FORGE test
expect_output  "failing test reported as FAIL" "FAIL" run_in "$D" $FORGE test

# ── 8. clean ──────────────────────────────────────────────────────────────────
echo ""
echo "=== clean ==="

D=$(use_fixture clean-test)
run_in "$D" $FORGE build > /dev/null 2>&1
expect_file    "build created .build/" "$D/.build/bin/ct"
run_in "$D" $FORGE clean
expect_no_file "clean removes .build/" "$D/.build"
expect_success "clean is idempotent" run_in "$D" $FORGE clean

touch "$D/tests/mymod.so"
touch "$D/tests/ext.cpython-312-x86_64-linux-gnu.so"
run_in "$D" $FORGE clean > /dev/null 2>&1
expect_no_file "clean removes .so from tests/" "$D/tests/mymod.so"
expect_no_file "clean removes versioned .so from tests/" "$D/tests/ext.cpython-312-x86_64-linux-gnu.so"

# ── 9. [[exe]] entries ────────────────────────────────────────────────────────
echo ""
echo "=== [[exe]] entries ==="

D=$(use_fixture exe-simple)
expect_success "[[exe]] builds" run_in "$D" $FORGE build
expect_file    "[[exe]] produces binary" "$D/.build/bin/hello"

D=$(use_fixture exe-custom-src)
expect_success "[[exe]] with custom src builds" run_in "$D" $FORGE build
expect_file    "[[exe]] custom src produces binary" "$D/.build/bin/greet"

D=$(use_fixture exe-with-dep)
expect_success "[[exe]] with lib dep links and runs" run_in "$D" $FORGE build
expect_file    "[[exe]] with dep produces binary" "$D/.build/bin/calc"
run_in "$D" $FORGE build > /dev/null 2>&1
expect_success "[[exe]] binary runs correctly" run_in "$D" .build/bin/calc

D=$(use_fixture exe-transitive)
expect_success "[[exe]] with transitive deps builds" run_in "$D" $FORGE build
expect_file    "[[exe]] transitive dep binary exists" "$D/.build/bin/app"
run_in "$D" $FORGE build > /dev/null 2>&1
expect_success "[[exe]] transitive deps resolve correctly" run_in "$D" .build/bin/app

D=$(use_fixture exe-baddep)
expect_failure "[[exe]] unknown dep → error" run_in "$D" $FORGE build
expect_output  "[[exe]] unknown dep message" "not declared" run_in "$D" $FORGE build
expect_no_file "[[exe]] unknown dep: no compilation attempted" "$D/.build"

D=$(use_fixture exe-noname)
expect_failure "[[exe]] with no name → error" run_in "$D" $FORGE build
expect_output  "[[exe]] no name error message" "no 'name'" run_in "$D" $FORGE build

D=$(use_fixture exe-multi)
expect_success "multiple [[exe]] entries build" run_in "$D" $FORGE build
expect_file    "first of multiple exes produced" "$D/.build/bin/foo"
expect_file    "second of multiple exes produced" "$D/.build/bin/bar"

D=$(use_fixture exe-multi-lib)
expect_success "multi-exe+multi-lib builds"              run_in "$D" $FORGE build
expect_file    "multi-exe+multi-lib: server produced"    "$D/.build/bin/server"
expect_file    "multi-exe+multi-lib: client produced"    "$D/.build/bin/client"
expect_file    "multi-exe+multi-lib: tool produced"      "$D/.build/bin/tool"
expect_file    "multi-exe+multi-lib: libcore.a produced" "$D/.build/lib/libcore.a"
expect_file    "multi-exe+multi-lib: libmath.a produced" "$D/.build/lib/libmath.a"
expect_file    "multi-exe+multi-lib: libio.a produced"   "$D/.build/lib/libio.a"
expect_file    "multi-exe+multi-lib: libengine.a produced" "$D/.build/lib/libengine.a"
run_in "$D" $FORGE build > /dev/null 2>&1
expect_success "multi-exe+multi-lib: server runs (full transitive closure)" run_in "$D" .build/bin/server
expect_success "multi-exe+multi-lib: client runs (partial dep subset)"      run_in "$D" .build/bin/client
expect_success "multi-exe+multi-lib: tool runs (single direct dep)"         run_in "$D" .build/bin/tool

# ── summary ───────────────────────────────────────────────────────────────────
echo ""
echo "================================"
printf "Results: %d passed, %d failed\n" "$PASS" "$FAIL"
echo "================================"
[ "$FAIL" -eq 0 ]
