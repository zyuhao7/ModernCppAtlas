CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -pedantic
BUILD_DIR := build/examples
SOURCES := $(wildcard examples/*/*.cpp)
BINARIES := $(patsubst examples/%.cpp,$(BUILD_DIR)/%,$(SOURCES))

.PHONY: all examples run verify clean

all: examples

examples: $(BINARIES)

$(BUILD_DIR)/%: examples/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -std=c++$(patsubst cpp%,%,$(firstword $(subst /, ,$*))) $< -o $@

run: examples
	@set -e; for exe in $(BINARIES); do \
		echo "== $$exe"; \
		"./$$exe"; \
	done

verify:
	uv run tools/verify_examples.py --compiler $(CXX) --run

clean:
	rm -rf build
