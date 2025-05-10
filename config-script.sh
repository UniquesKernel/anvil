#!/bin/bash
# config-script.sh - new version

if [ -z "$1" ]; then
  echo "Usage: $0 <NewModuleName>"
  exit 1
fi

MODULE_NAME=$1
MODULE_PATH="libs/$MODULE_NAME"

if [ -d "$MODULE_PATH" ]; then
  echo "Error: Module '$MODULE_NAME' already exists at '$MODULE_PATH'"
  exit 1
fi

echo "Creating module: $MODULE_NAME in $MODULE_PATH..."

mkdir -p "$MODULE_PATH/src"
mkdir -p "$MODULE_PATH/include"
mkdir -p "$MODULE_PATH/tests"

# Create template CMakeLists.txt for the module
cat > "$MODULE_PATH/CMakeLists.txt" <<EOL
set(MODULE_NAME $MODULE_NAME)
# ... (rest of the template module CMakeLists.txt content) ...
# Example for add_test:
if(BUILD_TESTING)
    # ... (add_library ${MODULE_NAME}_test_shared ...) ...
    add_test(
        NAME \${MODULE_NAME}_pytest
        COMMAND \"\${Python3_EXECUTABLE}\" -m pytest \"\${CMAKE_CURRENT_SOURCE_DIR}/tests/\"
        WORKING_DIRECTORY \"\${CMAKE_CURRENT_BINARY_DIR}\"
    )
    set_tests_properties(\${MODULE_NAME}_pytest PROPERTIES ENVIRONMENT "PYTHONPATH=\${CMAKE_CURRENT_BINARY_DIR}")
endif()
EOL

# Create placeholder C, H, and Python test files
touch "$MODULE_PATH/src/\${MODULE_NAME}.c"
touch "$MODULE_PATH/include/\${MODULE_NAME}.h"
touch "$MODULE_PATH/tests/test_\${MODULE_NAME}.py"
touch "$MODULE_PATH/tests/__init__.py" # If you want tests to be a package

echo "Module $MODULE_NAME created successfully."
echo "Next steps:"
echo "1. Implement your library in $MODULE_PATH/src/ and $MODULE_PATH/include/"
echo "2. Write Python tests in $MODULE_PATH/tests/"
echo "3. Add 'add_subdirectory($MODULE_PATH)' to your root CMakeLists.txt"