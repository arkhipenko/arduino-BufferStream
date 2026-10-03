# Host tests for BufferStream.
#   make               build and run the unit tests and the examples
#   make CXX=clang++ STD=c++17
# Each example's setup() runs on the host model and its output is compared with
# expected/<Example>.txt.

CXX ?= g++
STD ?= gnu++11
REPO := $(abspath ../..)

CXXFLAGS := -std=$(STD) -Wall -Wextra -Werror -g \
            -fsanitize=address,undefined -fno-sanitize-recover=all \
            -Istubs -I$(REPO)/src -DREPO_ROOT='"$(REPO)"'
SRCS := $(wildcard $(REPO)/src/*.cpp)
HDRS := $(wildcard $(REPO)/src/*.h) stubs/Arduino.h
EXAMPLES := ReadString WriteString ParseBuffer WriteOverflow

.PHONY: all test examples clean
all: test examples

test: test_bufferstream
	./test_bufferstream

test_bufferstream: test_bufferstream.cpp $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) test_bufferstream.cpp $(SRCS) -o $@

examples: $(addprefix example_,$(EXAMPLES))
	@for e in $(EXAMPLES); do \
	  ./example_$$e > example_$$e.out && diff -u expected/$$e.txt example_$$e.out && echo "pass example $$e" || exit 1; \
	done

.SECONDEXPANSION:
example_%: run_example.cpp $(SRCS) $(HDRS) $$(REPO)/examples/$$*/$$*.ino expected/$$*.txt
	$(CXX) $(CXXFLAGS) -DEXAMPLE_INO='"$(REPO)/examples/$*/$*.ino"' run_example.cpp $(SRCS) -o $@

clean:
	rm -f test_bufferstream example_*
