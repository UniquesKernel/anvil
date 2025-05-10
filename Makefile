.PHONY: help configure build test clean docs

BUILD_DIR = build
VENV_DIR = venv

help:
	@echo "Makefile Targets:"
	@echo "  venv         - Create Python virtual environment and install dependencies"
	@echo "  configure    - Configure the project with CMake (pass options via CMAKE_OPTS)"
	@echo "  build        - Build the project (after configuring)"
	@echo "  test         - Run tests with CTest (after building)"
	@echo "  docs         - Generate Doxygen documentation"
	@echo "  clean        - Clean the build directory"
	@echo "Example: make configure CMAKE_OPTS=\"-DBUILD_TESTING=ON\""

venv:
	@if [ ! -d "$(VENV_DIR)" ]; then python3 -m venv $(VENV_DIR); fi
	@$(VENV_DIR)/bin/pip install -r requirements.txt

configure:
	@cmake -B $(BUILD_DIR) $(CMAKE_OPTS) .

build:
	@cmake --build $(BUILD_DIR)

test:
	@cd $(BUILD_DIR) && ctest --output-on-failure

docs:
	@doxygen Doxyfile

clean:
	@rm -rf $(BUILD_DIR)
