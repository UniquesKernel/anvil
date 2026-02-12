#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -eq 0 ]]; then
  echo "Do not run this script as root. It will use sudo when needed."
  exit 1
fi

if ! command -v sudo >/dev/null 2>&1; then
  echo "sudo is required to install system packages."
  exit 1
fi

sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  doxygen \
  graphviz \
  python3 \
  python3-dev \
  python3-full \
  python3-venv \
  libpapi-dev \
  clang-tidy \
  cppcheck \
  iwyu
if [[ "${CC:-}" == *clang* ]] || [[ "${CXX:-}" == *clang* ]]; then
  sudo apt-get install -y clang
fi

if [[ ! -d ".venv" ]]; then
  python3 -m venv .venv
fi

.venv/bin/python -m pip install --upgrade pip
.venv/bin/python -m pip install -r requirements.txt
