# ltxpng: LaTeX fragment to PNG CLI tool
CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2 -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE
LDFLAGS =

SRC     = src/util.c src/args.c src/detect.c src/template.c src/run.c
OBJ     = $(SRC:.c=.o)
TEST_SRC = tests/test_util.c tests/test_args.c tests/test_detect.c \
           tests/test_template.c tests/test_run.c
TEST_BIN = $(TEST_SRC:.c=)

.PHONY: all debug test clean

all: ltxpng

ltxpng: src/main.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

debug: CFLAGS += -g -O0 -fsanitize=address -fno-omit-frame-pointer
debug: ltxpng

# Pattern rule for building test binaries.
# Each test binary links the test harness + the test file + the corresponding src/.o files.
tests/test_util: tests/test_util.c tests/test_harness.c src/util.c
	$(CC) $(CFLAGS) -I src -o $@ $^ $(LDFLAGS)

tests/test_args: tests/test_args.c tests/test_harness.c src/args.c src/util.c
	$(CC) $(CFLAGS) -I src -o $@ $^ $(LDFLAGS)

tests/test_detect: tests/test_detect.c tests/test_harness.c src/detect.c src/util.c
	$(CC) $(CFLAGS) -I src -o $@ $^ $(LDFLAGS)

tests/test_template: tests/test_template.c tests/test_harness.c src/template.c src/util.c src/detect.c
	$(CC) $(CFLAGS) -I src -o $@ $^ $(LDFLAGS)

tests/test_run: tests/test_run.c tests/test_harness.c src/run.c src/util.c
	$(CC) $(CFLAGS) -I src -o $@ $^ $(LDFLAGS)

test: $(TEST_BIN)
	@echo "=== Unit tests ==="
	@for t in $(TEST_BIN); do \
	  echo "--- $$t ---"; \
	  ./$$t || exit 1; \
	done
	@echo "=== All unit tests passed ==="
	@if [ -f tests/e2e.sh ] && [ -f ltxpng ]; then \
	  echo "=== End-to-end tests ==="; \
	  tests/e2e.sh ./ltxpng || exit 1; \
	  echo "=== All e2e tests passed ==="; \
	fi

clean:
	rm -f ltxpng
	rm -f src/*.o
	rm -f tests/test_util tests/test_args tests/test_detect tests/test_template tests/test_run
