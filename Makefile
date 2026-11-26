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
SRC_SRCS := $(filter-out src/semantic_old.c, $(SRC_SRCS))
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
	pandoc documentation/documentation.md -o documentation/dokumentace.pdf --pdf-engine=weasyprint --css=documentation/styles.css

e2e: build
	bash tests/e2e/tests.sh

export: pdf
	rm -rf result
	rm -f xjuricl01.zip
	mkdir result
	cp documentation/dokumentace.pdf result/
	cp -r src result/
	cp -r tests result/
	rm -f result/tests/e2e/ic26int-linux-x86_64
	rm -rf result/tests/e2e/tmp
	cp -r Makefile result/
	cp -r rozdeleni result/
	cp rozsireni result/
	cd result && zip -r ../xjuricl01.zip .
	rm -rf result
	./is_it_ok.sh xjuricl01.zip