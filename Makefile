EXEC_BIN = factorio-mod-diff

CC = gcc
C_FLAGS = -MMD -MP -Wall -Wextra

.DEFAULT_GOAL := all

# C_FLAGS += -DNO_WARN
# C_FLAGS += -DNO_COLOR

DEBUG_FLAGS = -g -fsanitize=address -O0 -fno-omit-frame-pointer
DEBUG_FLAGS += -DDEBUG
DEBUG_FLAGS += -DVERBOSE

RELEASE_FLAGS = -O2

BUILD_DIR = build

# LD_FLAGS := -l

#####################
#					#
#	   INCLUDES		#
#					#
#####################

# Find dirs with headers
INCLUDEDIRS := $(shell find src -type f -name '*.h' -exec dirname {} \; | sort -u)

# Prefix includes with -I
INCLUDES := $(addprefix -I, $(INCLUDEDIRS))

#####################
#					#
#	  SRC FILES		#
#					#
#####################

# Find all source files except main (so we can have different rules like test)
SRC := $(filter-out src/main.c, $(shell find src -type f -name '*.c'))

MAIN_SRC := src/main.c

#####################
#                   #
#      DEBUG        #
#                   #
#####################

DEBUG_DIR = $(BUILD_DIR)/debug

DEBUG_OBJ := $(patsubst src/%.c,$(DEBUG_DIR)/%.o,$(SRC))
DEBUG_MAIN_OBJ := $(patsubst src/%.c,$(DEBUG_DIR)/%.o,$(MAIN_SRC))

debug: $(DEBUG_OBJ) $(DEBUG_MAIN_OBJ)
	@$(CC) -o $(EXEC_BIN) $^ $(DEBUG_FLAGS) $(LD_FLAGS)

$(DEBUG_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) $(DEBUG_FLAGS) $(INCLUDES) -c $< -o $@

#####################
#                   #
#      RELEASE      #
#                   #
#####################

RELEASE_DIR = $(BUILD_DIR)/release

RELEASE_OBJ := $(patsubst src/%.c,$(RELEASE_DIR)/%.o,$(SRC))
RELEASE_MAIN_OBJ := $(patsubst src/%.c,$(RELEASE_DIR)/%.o,$(MAIN_SRC))

release: $(RELEASE_OBJ) $(RELEASE_MAIN_OBJ)
	@$(CC) -o $(EXEC_BIN) $^ $(RELEASE_FLAGS) $(LD_FLAGS)

$(RELEASE_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(C_FLAGS) $(RELEASE_FLAGS) $(INCLUDES) -c $< -o $@

#####################
#					#
#	   TARGETS     	#
#					#
#####################

all: release

run: release
	./$(EXEC_BIN)

run-debug: debug
	./$(EXEC_BIN)

clean:
	rm -rf $(BUILD_DIR) $(EXEC_BIN)

clean-debug:
	rm -rf $(DEBUG_DIR) $(EXEC_BIN)

clean-release:
	rm -rf $(RELEASE_DIR) $(EXEC_BIN)

clean-tests:
	$(MAKE) -C tests clean -s

tests:
	$(MAKE) -C tests -s

run-tests:
	$(MAKE) -C tests run-tests -s

.PHONY: all debug release run run-debug clean clean-debug clean-release clean-tests tests run-tests

# This makes header changes recompile
-include $(DEBUG_OBJ:.o=.d)
-include $(DEBUG_MAIN_OBJ:.o=.d)

-include $(RELEASE_OBJ:.o=.d)
-include $(RELEASE_MAIN_OBJ:.o=.d)
