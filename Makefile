#########################
#
# File: Makefile
# Author: xjurilu00, xoramst00, AI návrh architektury (chatGPT)
# Brief: Project makefile
#
#########################

.PHONY: build test format tidy docs dev pdf
CFLAGS = -Wall -Wextra -pedantic

SRC_SRCS := $(shell find src -name '*.c')
TEST_SRCS := $(shell find tests -name '*.c')
H_SRCS := $(shell find src tests -name '*.h')
ALL_SRCS := $(SRC_SRCS) $(TEST_SRCS) $(H_SRCS)

build:
	gcc $(CFLAGS) $(SRC_SRCS) -o Output

test:
	gcc $(CFLAGS) $(filter-out src/main.c,$(SRC_SRCS)) $(TEST_SRCS) -o test_runner
	./test_runner

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

export: pdf
	rm -rf result
	rm -f export.zip
	mkdir result
	cp documentation/dokumentace.pdf result/
	cp -r src result/
	cp -r tests result/
	cp -r Makefile result/
	cp -r rozdeleni result/
	cp rozsireni result/
	cd result && zip -r ../export.zip .
	rm -rf result
	./is_it_ok.sh export.zip