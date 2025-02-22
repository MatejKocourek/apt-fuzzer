# Directories

PROJECT_DIR ?= $(shell dirname $(PWD))
TASK1_DIR = $(PROJECT_DIR)/code-coverage
TASK2_DIR = $(PROJECT_DIR)/fuzzer
TASK4_DIR = $(PROJECT_DIR)/seed-generator
TASK4_BUILD_DIR = $(TASK4_DIR)/build
TASK4_BUILD_DIR_TEST = $(TASK4_DIR)/build/test
SEED_DIR ?= $(TASK2_DIR)/build/my-seeds

.PHONY: build test run clean

# The 'build' target builds the program using CMake
build:
	@echo "Building the program using CMake..."
	@mkdir -p $(TASK4_BUILD_DIR)
	@cd $(TASK4_BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# The 'test' target runs the test suite and generates the coverage report
test:
	@echo "Running tests and generating code coverage..."
	@mkdir -p $(TASK4_BUILD_DIR_TEST)
	@cmake -S . -B $(TASK4_BUILD_DIR_TEST) -DCOVERAGE=ON

	@echo "Running command and capturing output..."
	@OUTPUT=$$( \
	    { cmake --build $(TASK4_BUILD_DIR_TEST) --target coverage | tee /dev/stderr; } | \
	    grep -oE '[0-9]+\.[0-9]?%' | head -n 1 \
	) && \
	if [ -n "$$OUTPUT" ]; then \
	    echo "Extracted percentage: $$OUTPUT"; \
	    echo "$$OUTPUT" > coverage.txt; \
	else \
	    echo "No percentage found in the output" >&2; \
	fi

run:
	@echo "Generating seeds from files in $(FUZZED_PROG)"
	@$(TASK4_BUILD_DIR)/seed-generator $(FUZZED_PROG) $(FUZZED_PROG)/generated-seeds/

clean:
	@echo "Cleaning build..."
	@rm -fr $(TASK4_BUILD_DIR_TEST)
	@rm -fr $(TASK4_BUILD_DIR)
