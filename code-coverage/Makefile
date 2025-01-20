# Compiler and flags
CC = gcc

# Directories

PROJECT_DIR ?= $(shell dirname $(PWD))

BUILD_DIR = $(PROJECT_DIR)/code-coverage/build
BUILD_DIR_TEST = $(PROJECT_DIR)/code-coverage/build/test
TEST_DIR = $(PROJECT_DIR)/code-coverage/tests

.PHONY: build test benchmark run clean

# The 'build' target builds the program using CMake
build:
	@echo "Building the program using CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && make

# The 'test' target runs the test suite and generates the coverage report
test:
	@echo "Running tests and generating code coverage..."
	@mkdir -p $(BUILD_DIR_TEST)
	@cmake -S . -B $(BUILD_DIR_TEST) -DCOVERAGE=ON

	@echo "Running command and capturing output..."
	@OUTPUT=$$( \
	    { cmake --build $(BUILD_DIR_TEST) --target coverage | tee /dev/stderr; } | \
	    grep -oE '[0-9]+\.[0-9]?%' | head -n 1 \
	) && \
	if [ -n "$$OUTPUT" ]; then \
	    echo "Extracted percentage: $$OUTPUT"; \
	    echo "$$OUTPUT" > coverage.txt; \
	else \
	    echo "No percentage found in the output" >&2; \
	fi


# The 'benchmark' target runs a microbenchmark and prints the speedup
benchmark: build
	@echo "Annotating files for benchmarking..."
	@cd tests/fibonacci/ && $(BUILD_DIR)/code-coverage fibonacci.c
	@echo "Compiling files for benchmarking..."
	@$(CC) -O0 tests/fibonacci/0_instrumented_main.c -o tests/fibonacci/fibonacci-instr
	@$(CC) -O0 tests/fibonacci/fibonacci.c -o tests/fibonacci/fibonacci-og
	@echo "Running benchmarks..."

	@echo "Running PROGRAM_V1..."
	@START_V1=$$(date +%s%3N) && \
	tests/fibonacci/fibonacci-og 450000000 > /dev/null && \
	END_V1=$$(date +%s%3N) && \
	TIME_V1=$$((END_V1 - START_V1)) && \
	echo "Time for PROGRAM_V1: $$TIME_V1 milliseconds" && \
	echo "Running PROGRAM_V2..." && \
	START_V2=$$(date +%s%3N) && \
	tests/fibonacci/fibonacci-instr 450000000 > /dev/null && \
	END_V2=$$(date +%s%3N) && \
	TIME_V2=$$((END_V2 - START_V2)) && \
	echo "Time for PROGRAM_V2: $$TIME_V2 milliseconds" && \
	SLOWDOWN=$$(((TIME_V2*100) / (TIME_V1))); \
	SLOWDOWN0=$$((SLOWDOWN / 100)); \
	SLOWDOWN1=$$((SLOWDOWN % 100)); \
	echo "Slow-down: $$SLOWDOWN0.$$SLOWDOWN1";

	@rm tests/fibonacci/0_instrumented_main.c
	@rm tests/fibonacci/fibonacci-instr
	@rm tests/fibonacci/fibonacci-og

# The 'run' target runs the code coverage tool on the C file(s) specified by the TARGET_COV environment variable
run: build
	@echo "Running code coverage tool on files in $(TARGET_COV)/*.c"
	@cd $(TARGET_COV) && $(BUILD_DIR)/code-coverage *.c
	@cd $(TARGET_COV) && $(CC) *_instrumented_main.c -o instr_prog
	@cd $(TARGET_COV) && ./instr_prog $(ARGS_COV)
	@cd $(TARGET_COV) && cat coverage.lcov
clean:
	@echo "Cleaning build..."
	@rm -fr $(BUILD_DIR_TEST)
	@rm -fr $(BUILD_DIR)
