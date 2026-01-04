# Makefile for FastTrig Library

# Compiler settings
CXX := g++
CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -I include

# ARM cross-compilation (uncomment for embedded targets)
# CXX := arm-none-eabi-g++
# CXXFLAGS += -mcpu=cortex-m4 -mthumb

# Directories
SRC_DIR := .
BUILD_DIR := build
BIN_DIR := bin

# Create directories
$(shell mkdir -p $(BUILD_DIR) $(BIN_DIR))

# Targets
EXAMPLES := $(BIN_DIR)/examples
TESTS := $(BIN_DIR)/test_fast_trig

# Default target
all: $(EXAMPLES) $(TESTS)

# Build examples
$(EXAMPLES): examples/examples.cpp include/fast_trig.hpp
	@echo "Building examples..."
	$(CXX) $(CXXFLAGS) $< -o $@
	@echo "Examples built: $@"

# Build tests
$(TESTS): tests/test_fast_trig.cpp include/fast_trig.hpp
	@echo "Building tests..."
	$(CXX) $(CXXFLAGS) $< -o $@ -lm
	@echo "Tests built: $@"

# Run examples
run-examples: $(EXAMPLES)
	@echo "Running examples..."
	@$(EXAMPLES)

# Run tests
test: $(TESTS)
	@echo "Running tests..."
	@$(TESTS)

# Benchmark
benchmark: $(EXAMPLES)
	@echo "Running benchmarks..."
	@$(EXAMPLES) | grep -A 10 "Performance Benchmark"

# Clean build artifacts
clean:
	@echo "Cleaning..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Install header to system (requires sudo)
install:
	@echo "Installing header..."
	install -D -m 644 include/fast_trig.hpp /usr/local/include/fast_trig.hpp
	@echo "Installed to /usr/local/include/"

# Uninstall
uninstall:
	@echo "Uninstalling..."
	rm -f /usr/local/include/fast_trig.hpp

# Build for different precision levels
precision-test: include/fast_trig.hpp
	@echo "Testing different precision levels..."
	@for size in 32 64 128 256 512; do \
		echo "Building with table size $$size..."; \
		$(CXX) $(CXXFLAGS) -DTABLE_SIZE=$$size tests/test_fast_trig.cpp -o $(BIN_DIR)/test_$$size -lm; \
	done

# Static analysis
analyze:
	@echo "Running static analysis..."
	cppcheck --enable=all --std=c++20 include/fast_trig.hpp examples/ tests/

# Format code (requires clang-format)
format:
	@echo "Formatting code..."
	clang-format -i include/fast_trig.hpp examples/*.cpp tests/*.cpp

# Generate assembly output for inspection
asm: examples/examples.cpp include/fast_trig.hpp
	@echo "Generating assembly..."
	$(CXX) $(CXXFLAGS) -S -fverbose-asm $< -o $(BUILD_DIR)/examples.s
	@echo "Assembly output: $(BUILD_DIR)/examples.s"

# Help target
help:
	@echo "FastTrig Library Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build examples and tests (default)"
	@echo "  test         - Build and run tests"
	@echo "  run-examples - Build and run examples"
	@echo "  benchmark    - Run performance benchmarks"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install header to /usr/local/include"
	@echo "  uninstall    - Remove installed header"
	@echo "  analyze      - Run static analysis"
	@echo "  format       - Format source code"
	@echo "  asm          - Generate assembly output"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CXX          - C++ compiler (default: g++)"
	@echo "  CXXFLAGS     - Compiler flags"
	@echo ""
	@echo "Examples:"
	@echo "  make                     # Build all"
	@echo "  make test               # Run tests"
	@echo "  make CXX=clang++        # Build with clang"

.PHONY: all test run-examples benchmark clean install uninstall precision-test analyze format asm help
