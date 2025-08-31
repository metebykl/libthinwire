CFLAGS=-Wall -Wextra

.PHONY: all
all: examples

.PHONY: examples
examples: 
	$(MAKE) -C examples/