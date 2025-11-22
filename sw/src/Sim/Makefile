PROJECT   = FluidSim
BUILD_DIR = build
TARGET    = $(PROJECT).elf

CC = gcc

LIB_DIR = mpsLibC
LIB     = $(LIB_DIR)/build/$(LIB_DIR).a

OS_NAME := $(shell uname -s)

C_FLAGS = \
	-Wextra \
	-Wall   \
	-O0     \
	-g3     \
	-I/opt/homebrew/include \
	-Iinc

LD_FLAGS = \
	-L/opt/homebrew/lib \
	-lm \
	-lpthread \
	-lglfw

SRC_FILES = \
	src/PressureSolver.c \
	src/Conroller.c

OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

.PHONY: clean all

all: $(TARGET)

# build library
$(LIB):
	$(MAKE) -C $(LIB_DIR) all

# final binary links against library
$(TARGET): $(OBJS) $(LIB) | $(BUILD_DIR)
	$(CC) $(OBJS) $(LIB) -o $@ $(LD_FLAGS)

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	$(MAKE) -C $(LIB_DIR) clean
