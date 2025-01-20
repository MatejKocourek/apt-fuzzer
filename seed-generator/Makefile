# Directories

PROJECT_DIR ?= $(shell dirname $(PWD))
TASK1_DIR = $(PROJECT_DIR)/code-coverage
TASK2_DIR = $(PROJECT_DIR)/fuzzer
TASK4_DIR = $(PROJECT_DIR)/seed-generator
TASK4_BUILD_DIR = $(TASK4_DIR)/build
TASK4_BUILD_DIR_TEST = $(TASK4_DIR)/build/test
SEED_DIR ?= $(TASK2_DIR)/build/my-seeds

# The 'build' target builds the program using CMake
build:
	@echo "Creating symlinks from the first task"
	@ln -s $(TASK1_DIR)/cpp-tree-sitter $(TASK4_DIR)/cpp-tree-sitter
	@ln -s $(TASK1_DIR)/tree-sitter $(TASK4_DIR)/tree-sitter
	@ln -s $(TASK1_DIR)/tree-sitter-c $(TASK4_DIR)/tree-sitter-c
	@ln -s $(TASK1_DIR)/symbol-identifiers.h $(TASK4_DIR)/symbol-identifiers.h
	@echo "Building the program using CMake..."
	@mkdir -p $(TASK4_BUILD_DIR)
	@cd $(TASK4_BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release .. && make

# The 'test' target runs the test suite and generates the coverage report
test:
	@echo "Running tests and generating code coverage..."
	@mkdir -p $(TASK4_BUILD_DIR_TEST)
	@cmake -S . -B $(TASK4_BUILD_DIR_TEST) -DCOVERAGE=ON
	@cmake --build $(TASK4_BUILD_DIR_TEST) --target coverage

run: build
	@echo "Generating seeds from files in $(FUZZED_PROG)"
	@$(TASK4_BUILD_DIR)/seed-generator $(FUZZED_PROG) $(FUZZED_PROG)/generated-seeds/

clean:
	@echo "Cleaning build..."
	@rm -fr $(TASK4_BUILD_DIR_TEST)
	@rm -f $(TASK4_DIR)/cpp-tree-sitter
	@rm -f $(TASK4_DIR)/tree-sitter
	@rm -f $(TASK4_DIR)/tree-sitter-c
	@rm -f $(TASK4_DIR)/symbol-identifiers.h
	@rm -fr $(TASK4_BUILD_DIR)
