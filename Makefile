CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -w
CC       ?= gcc
CFLAGS   ?= -O2

PEGTL    := lib/PEGTL/include/tao/pegtl.hpp
LANGS    := L1 L2 L3 IR
PLATFORM := $(shell uname -s)-$(shell uname -m)
BUILD    := build/$(PLATFORM)
BINS     := $(addprefix $(BUILD)/bin/,$(LANGS))

ifeq ($(shell uname -m),x86_64)
BINS     += $(BUILD)/runtime.o
endif

.PHONY: all test clean distclean

all: $(BINS)

$(PEGTL):
	@scripts/bootstrap.sh

define LANG_template
$(1)_SRCS := $$(wildcard $(1)/src/*.cpp)
$(1)_OBJS := $$(patsubst $(1)/src/%.cpp,$(BUILD)/$(1)/%.o,$$($(1)_SRCS))

$(BUILD)/$(1)/%.o: $(1)/src/%.cpp $(PEGTL)
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) -MMD -MP -I$(1)/src -Ilib/PEGTL/include -c $$< -o $$@

$(BUILD)/bin/$(1): $$($(1)_OBJS)
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) $$^ -o $$@

-include $$($(1)_OBJS:.o=.d)
endef

$(foreach L,$(LANGS),$(eval $(call LANG_template,$(L))))

$(BUILD)/runtime.o: runtime/runtime.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test: all
	@scripts/test $(T)

clean:
	rm -rf build

distclean: clean
	rm -rf lib
