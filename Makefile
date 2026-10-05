#########################
#
# File: Makefile
# Author: xjurilu00, xoramst00, AI návrh architektury (chatGPT)
# Brief: Project makefile
#
#########################

.PHONY: build format tidy docs dev
CFLAGS = -Wall -Wextra -pedantic

C_SRCS := $(shell find src tests -name '*.c')
H_SRCS := $(shell find src tests -name '*.h')
ALL_SRCS := $(C_SRCS) $(H_SRCS)

build:
	gcc $(CFLAGS) $(C_SRCS) -o Output

dev: format tidy build

docs:
	doxygen Doxyfile
	xdg-open docs/html/index.html

format:
	clang-format -i $(ALL_SRCS)

tidy:
	clang-tidy -checks='bugprone-*,clang-analyzer-*,readability-*,portability-*' $(ALL_SRCS) -- $(CFLAGS)