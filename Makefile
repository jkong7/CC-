CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -w
CC       ?= gcc
CFLAGS   ?= -O2

PEGTL    := lib/PEGTL/include/tao/pegtl.hpp
LANGS    := L1 L2 L3 IR
BINS     := $(addprefix bin/,$(LANGS))

.PHONY: all clean distclean

all: $(BINS)

$(PEGTL):
	@scripts/bootstrap.sh

define LANG_template
$(1)_SRCS := $$(wildcard $(1)/src/*.cpp)
$(1)_OBJS := $$(patsubst $(1)/src/%.cpp,build/$(1)/%.o,$$($(1)_SRCS))

build/$(1)/%.o: $(1)/src/%.cpp $(PEGTL)
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(CXXFLAGS) -MMD -MP -I$(1)/src -Ilib/PEGTL/include -c $$< -o $$@

bin/$(1): $$($(1)_OBJS)
	@mkdir -p bin
	$$(CXX) $$(CXXFLAGS) $$^ -o $$@

-include $$($(1)_OBJS:.o=.d)
endef

$(foreach L,$(LANGS),$(eval $(call LANG_template,$(L))))

clean:
	rm -rf build bin

distclean: clean
	rm -rf lib
