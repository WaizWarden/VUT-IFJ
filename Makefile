.PHONY: format tidy docs generate-docs build dev
CFLAGS = -Wall -Wextra -pedantic

# Find all .c and .h files in src folder
SRCS := $(shell find src -name '*.c' -o -name '*.h')

build: $(SRCS)
	gcc $(CFLAGS) $(SRCS) -o Output

dev: format tidy build

docs:
	doxygen Doxyfile
	xdg-open docs/html/index.html

format:
	clang-format -i $(SRCS)

tidy:
	clang-tidy -checks='*' $(SRCS) -- $(CFLAGS)