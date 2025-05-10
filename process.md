# Anvil Project Template

This repository serves as a professional template for C/Assembly projects in the Anvil ecosystem. It provides a complete development environment with support for property-based testing, documentation generation, and professional package management.

## Features

The template includes a comprehensive build system that supports:
- CMake-based project configuration with proper library packaging
- Development and system-wide installation management
- Property-based testing using Python's Hypothesis framework
- Memory leak detection and debugging tools
- Automated documentation generation with Doxygen
- Professional package creation for distribution

## Prerequisites

The following tools are required to use this template:
- Ubuntu-based system
- GCC or Clang compiler
- CMake (version 3.22 or higher)
- Python 3 with pip and venv
- Valgrind for memory analysis
- Doxygen for documentation generation
- Git for version control

## Quick Start

1. Set up the development environment:
   ```bash
   chmod +x setup.sh
   ./setup.sh
   ```

2. Configure the project structure:
   ```bash
   chmod +x config-script.sh
   ./config-script.sh
   ```

3. Set up your development installation directory:
   ```bash
   make setup-dev
   source setup-dev-env.sh
   ```

## Project Structure

```
project/
├── cmake/                  # CMake modules and configuration
│   ├── CompilerStandards.cmake
│   ├── Functions.cmake
│   └── {project}-config.cmake.in  #rename to ${project_name}-config.cmake.in
├── include/               # Public header files
│   └── anvil/            # Namespace-organized headers
├── src/                  # Implementation files
│   ├── core/            # Core C implementation
│   └── asm/             # Assembly implementations
├── tests/               # Test suite
│   └── python/         # Hypothesis-based tests
└── docs/               # Documentation
```

## Build System

The build system provides several important targets:

### Basic Build Operations
- `make build` - Build the project
- `make clean` - Clean build artifacts
- `make clean-all` - Clean all generated files and environments

### Installation Management
- `make install PREFIX=/path` - Install to system location
- `make install-dev` - Install to development directory
- `make setup-dev` - Configure development environment

### Testing and Analysis
- `make test` - Run the test suite
- `make test-memcheck` - Run tests with memory checking
- `make memcheck` - Check for memory leaks
- `make debug` - Debug with GDB

### Documentation and Packaging
- `make docs` - Generate documentation
- `make package` - Create distribution package

## Development Workflow

This template is designed for professional library development. Here's how to use it effectively:

1. **Library Development**:
   ```bash
   # Set up development environment
   make setup-dev
   source setup-dev-env.sh
   
   # Build and install to development directory
   make install-dev
   ```

2. **Testing Changes**:
   ```bash
   # Run the test suite
   make test
   
   # Check for memory leaks
   make test-memcheck
   ```

3. **Creating Releases**:
   ```bash
   # Update version number
   VERSION=1.0.0 make package
   ```

## Configuration

The build system can be configured through several variables:

- `VERSION` - Project version (default: 0.1.0)
- `PREFIX` - System installation prefix (default: /usr/local)
- `DEV_PREFIX` - Development installation prefix (default: ~/anvil-dev)
- `BUILD_TESTING` - Enable/disable test builds (ON/OFF)

## Integration

This template is designed to work seamlessly with other libraries in the Anvil ecosystem. Libraries are packaged with proper CMake configuration files, making them easy to find and use in other projects:

```cmake
find_package(anvil-yourlib REQUIRED)
target_link_libraries(your_target PRIVATE Anvil::yourlib)
```

## Testing

The project builds a **shared library** (`libproject_test.so`) specifically for Python-based testing. This library:
- Includes debug symbols (`-g3`) and assertions (`_DEBUG`)
- Exposes internal symbols for direct testing via Python's `ctypes`
- Is excluded from production builds

## Documentation

The template uses Doxygen for documentation generation. Document your code using Doxygen-style comments, and generate documentation using:

```bash
make docs
```

The documentation will be available in `docs/doxygen/html`.

## LSP Support

The template includes built-in support for Language Server Protocol:
- Generates compile_commands.json for clangd
- Configures include paths for IDE support
- Sets up proper compiler flags

## Contributing

Before contributing:
1. Ensure all tests pass: `make test`
2. Check for memory leaks: `make test-memcheck`
3. Generate and review documentation: `make docs`
4. Create a distribution package: `make package`
