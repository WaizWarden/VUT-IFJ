#########################
#
# IFJ26 compiler
#
# File: Makefile
# Author: xjuricl01, xoramst00, AI návrh architektury + valgrind cíl (chatGPT)
# Brief: Project makefile
#
#########################

.PHONY: build test valgrind format tidy docs dev pdf docker docker-build
CFLAGS = -Wall -Wextra -pedantic

CONTAINER_ENGINE ?= $(shell command -v docker 2>/dev/null || command -v podman 2>/dev/null)
DOCKER_IMAGE ?= ifj26-dev

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
	@if [ -n "$$DISPLAY" ] && command -v xdg-open >/dev/null 2>&1; then \
		xdg-open docs/html/index.html; \
	else \
		printf 'Documentation generated at docs/html/index.html\n'; \
	fi

format:
	clang-format -i $(ALL_SRCS)

tidy:
	clang-tidy $(ALL_SRCS) -- $(CFLAGS)

pdf:
	pandoc --defaults=documentation/pandoc.yaml

e2e: build
	bash tests/e2e/tests.sh

valgrind: CFLAGS += -g
valgrind: build
	@command -v valgrind >/dev/null 2>&1 || { echo "Valgrind is required for this target" >&2; exit 127; }
	@set -e; \
	for source in tests/e2e/basic/*.scala tests/e2e/custom/*.scala; do \
		printf 'Valgrind: %s\n' "$$source"; \
		valgrind --leak-check=full --show-leak-kinds=all \
			--errors-for-leak-kinds=definite,indirect --track-origins=yes \
			--error-exitcode=1 ./Output < "$$source" > /dev/null; \
	done

docker: docker-build
	$(CONTAINER_ENGINE) run --rm -it \
		$(if $(findstring podman,$(notdir $(CONTAINER_ENGINE))),--userns=keep-id,) \
		--user "$$(id -u):$$(id -g)" \
		-e HOME=/tmp \
		--volume "$(CURDIR):/workspace:z" \
		-w /workspace $(DOCKER_IMAGE) bash

docker-build:
	@if [ -z "$(CONTAINER_ENGINE)" ]; then \
		echo "Docker or Podman is required for this target" >&2; exit 127; \
	fi
	$(CONTAINER_ENGINE) build -f Dockerfile.dev -t $(DOCKER_IMAGE) .

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