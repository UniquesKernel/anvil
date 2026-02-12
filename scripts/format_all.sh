#!/bin/bash

find . -type f \( \
    -name "*.c" -o \
    -name "*.cpp" -o \
    -name "*.cc" -o \
    -name "*.cxx" -o \
    -name "*.h" -o \
    -name "*.hpp" -o \
    -name "*.hxx" \
\) -not -path "*/build/*" \
   -not -path "*/.git/*" \
   -print0 | xargs -0 -P $(nproc) -n 10 clang-format -i

echo "Formatting complete!"
