# Compiler and flags
CC = gcc

# Directories

PROJECT_DIR ?= $(shell dirname $(PWD))

BUILD_DIR = $(PROJECT_DIR)/fuzzer/build
BUILD_DIR_TEST = $(PROJECT_DIR)/fuzzer/build/test

# The 'build' target builds the program using CMake
build:
	@echo "Building the program using CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# The 'test' target runs the test suite and generates the coverage report
test:
	@echo "Running tests and generating code coverage..."
	@mkdir -p $(BUILD_DIR_TEST)
	@cmake -S . -B $(BUILD_DIR_TEST) -DCOVERAGE=ON
	@cmake --build $(BUILD_DIR_TEST) --target coverage

# The 'run' target runs the code coverage tool on the C file(s) specified by the TARGET_COV environment variable
run: build
	@echo "Build dir is $(BUILD_DIR)"
	@echo "Running fuzzer on $(FUZZED_PROG) and placing results to $(RESULT_FUZZ)"
	@cd $(BUILD_DIR) && ./fuzzer $(FUZZED_PROG) $(RESULT_FUZZ) $(MINIMIZE) $(INPUT)
clean:
	@echo "Cleaning build..."
	@rm -fr $(BUILD_DIR)
	@rm -fr $(BUILD_DIR_TEST)
