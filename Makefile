CFLAGS=-Wall -Wextra

.PHONY: all
all: tests examples

.PHONY: tests
tests: 
	$(MAKE) -C tests/

.PHONY: examples
examples: 
	$(MAKE) -C examples/