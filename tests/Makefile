CC := gcc
CFLAGS := -Wall -Wextra -g -fsanitize=address -O0 -fno-omit-frame-pointer
CFLAGS := -DDEBUG -DVERBOSE

BUILD_DIR := ../build/tests

SRC := $(shell find ../src -type f -name '*.c' \
		! -path '../tests/test_*' \
		! -name 'main.c')

INCLUDEDIRS := $(shell find .. -type d)
INCLUDES := $(addprefix -I,$(INCLUDEDIRS))

TEST_SRC := $(wildcard test_*.c)
TESTS := $(patsubst %.c,$(BUILD_DIR)/%,$(TEST_SRC))

.PHONY: all run-tests clean

all: $(TESTS)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%: %.c $(SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(SRC) -o $@

run-tests: $(TESTS)
	@echo "Running $(words $(TESTS)) unit-tests"
	@passed=0;                                                                  \
	for test in $(TESTS); do                                                    \
		echo "\n=== $$(basename $$test) ===";                                   \
		$$test;                                                                 \
		status=$$?;                                                             \
		if [ $$status -ne 0 ]; then                                             \
			echo "\e[31mFAILED\e[0m $$(basename $$test) (exit code: $$status)"; \
		else                                                                    \
			echo "\e[32mPASSED\e[0m $$(basename $$test)";                       \
			passed=$$((passed + 1));                                            \
		fi;                                                                     \
	done;                                                                       \
	echo "Passed $$passed/$(words $(TESTS)) tests";

clean:
	rm -rf $(BUILD_DIR)
