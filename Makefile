CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -pedantic
BUILD_DIR := build/examples
SOURCES := $(wildcard examples/*/*.cpp)
BINARIES := $(patsubst examples/%.cpp,$(BUILD_DIR)/%,$(SOURCES))

shell_quote = '$(subst ','"'"',$(1))'
CONFIG_HASH := $(firstword $(shell printf '%s\0%s\0' $(call shell_quote,$(CXX)) $(call shell_quote,$(CXXFLAGS)) | sha256sum))
COMPILED_DIR := $(BUILD_DIR)/.compiled/$(CONFIG_HASH)
refresh_example_link = $(if $(filter $(abspath $(COMPILED_DIR)/$*),$(realpath $@)),,FORCE)

.PHONY: all examples run verify clean FORCE
.SECONDEXPANSION:
.PRECIOUS: $(COMPILED_DIR)/%

all: examples

examples: $(BINARIES)

# Keep the public example paths and refresh them when the configuration changes.
$(BUILD_DIR)/%: $(COMPILED_DIR)/% $$(refresh_example_link)
	@mkdir -p "$(@D)"
	@ln -sfnT "../.compiled/$(CONFIG_HASH)/$*" "$@"

$(COMPILED_DIR)/%: examples/%.cpp Makefile
	@mkdir -p "$(@D)"
	@set -e; \
		compile_tmp=$$(mktemp -d "$(@D)/.compile.XXXXXX"); \
		trap 'rm -rf -- "$$compile_tmp"' EXIT; \
		(set -x; $(CXX) $(CXXFLAGS) -std=c++$(patsubst cpp%,%,$(firstword $(subst /, ,$*))) "$<" -o "$$compile_tmp/$(@F)"); \
		mv -fT "$$compile_tmp/$(@F)" "$@"

run: examples
	@set -e; for exe in $(BINARIES); do \
		echo "== $$exe"; \
		"$(COMPILED_DIR)/$${exe#$(BUILD_DIR)/}"; \
	done

verify:
	uv run tools/verify_examples.py --compiler $(CXX) --run

clean:
	rm -rf build
