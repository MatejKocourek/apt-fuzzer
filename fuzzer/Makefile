# Compiler and flags
CC = gcc

# Directories

PROJECT_DIR ?= $(shell dirname $(PWD))
TASK1_DIR = $(PROJECT_DIR)/code-coverage
TASK2_DIR = $(PROJECT_DIR)/fuzzer
TASK4_DIR = $(PROJECT_DIR)/seed-generator

BUILD_DIR = $(TASK2_DIR)/build
BUILD_DIR_TEST = $(TASK2_DIR)/build/test

CRAFTED_SEEDS = $(PROJECT_DIR)/fuzzer/seeds/

POWER_SCHEDULE ?= boosted

.PHONY: build test blackbox greybox greybox-smarter prepare-coverage prepare-seeds run clean

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

	@echo "Running mutation testing"
	@clang++-17 $(PROJECT_DIR)/fuzzer/tests-unit.cpp -g -std=c++20 -fpass-plugin=/usr/lib/mull-ir-frontend-17 -grecord-command-line -lgtest -lgtest_main -o $(BUILD_DIR)/fuzzer-mutated

	@OUTPUT=$$( \
	    { mull-runner-17 $(BUILD_DIR)/fuzzer-mutated --mutation-score-threshold=50 --no-output | tee /dev/stderr; } | \
	    grep -oE '[0-9]+%' | tail -n 1 \
	) && \
	if [ -n "$$OUTPUT" ]; then \
	    echo "Extracted percentage: $$OUTPUT"; \
	    echo "$$OUTPUT" > mutation.txt; \
	else \
	    echo "No percentage found in the output" >&2; \
	fi


# The 'run' target runs the code coverage tool on the C file(s) specified by the TARGET_COV environment variable
blackbox:
	@echo "Running blacbox fuzzer on $(FUZZED_PROG) and placing results to $(RESULT_FUZZ)"
	@cd $(BUILD_DIR) && ./fuzzer $(FUZZED_PROG) $(RESULT_FUZZ) $(MINIMIZE) $(INPUT) $(TIMEOUT) $(NB_KNOWN_BUGS)

prepare-coverage:
	@echo "Running code coverage tool on files in $(FUZZED_PROG)/*.c"
	@cd $(FUZZED_PROG) && $(BUILD_DIR)/../../code-coverage/build/code-coverage *.c
	@cd $(FUZZED_PROG) && $(CC) *_instrumented_main.c -fsanitize=address -g -O0 -lm -o instr_prog
	@rm $(FUZZED_PROG)/*_instrumented_main.c

greybox:
	@$(MAKE) prepare-coverage
	@echo "Running greybox fuzzer on $(FUZZED_PROG) and placing results to $(RESULT_FUZZ)"
ifdef INPUT_SEEDS
	@cd $(FUZZED_PROG) && $(BUILD_DIR)/fuzzer instr_prog $(RESULT_FUZZ) $(MINIMIZE) $(INPUT) $(TIMEOUT) $(NB_KNOWN_BUGS) $(POWER_SCHEDULE) coverage.lcov 50 25 $(INPUT_SEEDS)
else
	@cd $(FUZZED_PROG) && $(BUILD_DIR)/fuzzer instr_prog $(RESULT_FUZZ) $(MINIMIZE) $(INPUT) $(TIMEOUT) $(NB_KNOWN_BUGS) $(POWER_SCHEDULE) coverage.lcov 50 25 $(CRAFTED_SEEDS)
endif
	@rm $(FUZZED_PROG)/instr_prog

greybox-smarter:
	@echo "Running a smarter version of greybox fuzzer with analysis of the source code"
	@$(MAKE) prepare-seeds
	@$(MAKE) prepare-coverage
	@cd $(FUZZED_PROG) && $(BUILD_DIR)/fuzzer instr_prog $(RESULT_FUZZ) $(MINIMIZE) $(INPUT) $(TIMEOUT) 0 $(POWER_SCHEDULE) coverage.lcov 0 75 $(FUZZED_PROG)/generated-seeds/

prepare-seeds:
	@echo "Generating interesting seeds from source files..."
	@mkdir -p $(FUZZED_PROG)/generated-seeds/
	@cd $(TASK4_DIR) && $(MAKE) run
	@if [ -d "$(INPUT_SEEDS)" ]; then echo "Directory $(INPUT_SEEDS) exists. Copying contents from $(INPUT_SEEDS) to $(FUZZED_PROG)/generated-seeds/ ...";cp $(INPUT_SEEDS)/* $(FUZZED_PROG)/generated-seeds/; fi

run:
	@if [ "$(FUZZER)" = "blackbox" ]; then \
		$(MAKE) blackbox; \
	elif [ "$(FUZZER)" = "greybox" ]; then \
		$(MAKE) greybox; \
	else \
		$(MAKE) greybox-smarter; \
	fi

clean:
	@echo "Cleaning build..."
	@rm -fr $(BUILD_DIR)
	@rm -fr $(BUILD_DIR_TEST)
