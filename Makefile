#########################
#
# File: Makefile
# Author: xjurilu00, xoramst00, AI návrh architektury (chatGPT)
# Brief: Project makefile
#
#########################

.PHONY: build format tidy docs dev pdf, export
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
	clang-tidy $(ALL_SRCS) -- $(CFLAGS)

pdf:
	pandoc documentation/documentation.md -o documentation/documentation.pdf --pdf-engine=weasyprint --css=documentation/styles.css