.PHONY: help venv configure build test docs clean

BUILD_DIR = build
VENV_DIR = .venv

# Allow overriding compilers, e.g. CC=clang CXX=clang++ make build
ifdef CC
  CMAKE_OPTS += -DCMAKE_C_COMPILER=$(CC)
endif
ifdef CXX
  CMAKE_OPTS += -DCMAKE_CXX_COMPILER=$(CXX)
endif

# Prefer the project venv Python when available so tests use installed deps.
VENV_PY = $(VENV_DIR)/bin/python
ifneq ($(wildcard $(VENV_PY)),)
	CMAKE_OPTS += -DPython3_EXECUTABLE=$(abspath $(VENV_PY))
endif

help:
	@echo "Makefile Targets:"
	@echo "  venv          - Create Python virtual environment and install dependencies"
	@echo "  configure     - Configure the project with CMake (pass options via CMAKE_OPTS)"
	@echo "  build         - Build the project (after configuring)"
	@echo "  test          - Run tests with CTest (after building)"
	@echo "  docs          - Generate Doxygen documentation"
	@echo "  clean         - Clean build and docs output"

venv:
	@if [ ! -d "$(VENV_DIR)" ]; then python3 -m venv $(VENV_DIR); fi
	@$(VENV_DIR)/bin/pip install -r requirements.txt

configure:
	@cmake -B $(BUILD_DIR) $(CMAKE_OPTS) .

build: configure
	@cmake --build $(BUILD_DIR)

test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

docs:
	@doxygen Doxyfile

clean:
	@rm -rf $(BUILD_DIR) docs/doxygen
